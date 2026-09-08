#include "spark/scene/assets/gltf/GltfSkinNodeLoader.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Transform.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/assets/gltf/GltfSkinnedMeshBuilder.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

#include "cgltf.h"

#include <cmath>

namespace Spark {

namespace {

std::uint32_t BaseColorTexCoordSet(const cgltf_primitive* prim) {
    if (prim == nullptr || prim->material == nullptr || !prim->material->has_pbr_metallic_roughness) {
        return 0;
    }
    return static_cast<std::uint32_t>(prim->material->pbr_metallic_roughness.base_color_texture.texcoord);
}

std::uint32_t JointIndexForNode(const cgltf_skin* skin, cgltf_node* node) {
    if (skin == nullptr || node == nullptr) {
        return ~0u;
    }
    for (cgltf_size i = 0; i < skin->joints_count; ++i) {
        if (skin->joints[i] == node) {
            return static_cast<std::uint32_t>(i);
        }
    }
    return ~0u;
}

Transform NodeLocalTransform(const cgltf_node* node) {
    Transform t{};
    if (node == nullptr) {
        return t;
    }
    if (node->has_translation) {
        t.translation = {node->translation[0], node->translation[1], node->translation[2]};
    }
    if (node->has_rotation) {
        t.rotation = {node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]};
    }
    if (node->has_scale) {
        t.scale = {node->scale[0], node->scale[1], node->scale[2]};
    } else {
        t.scale = Vector3::One;
    }
    return t;
}

bool ShouldFlipGltfTriangleWinding(const Matrix4& bakeWorld) noexcept {
    return bakeWorld.DeterminantUpper3x3() >= 0.0F;
}

/** cgltf / glTF matrices are column-major; Spark::Matrix4 matches (same as mesh_gltf.cpp). */
Matrix4 Matrix4FromCgltf(const cgltf_float* cm) noexcept {
    Matrix4 out{};
    for (int i = 0; i < 16; ++i) {
        out.m[i] = static_cast<float>(cm[i]);
    }
    return out;
}

}  // namespace

GltfSkinNodeBuildResult TryBuildSkinNode(
        const cgltf_data* data,
        cgltf_node* skinNode,
        const char* sourcePath) noexcept {
    GltfSkinNodeBuildResult result{};
    if (data == nullptr || skinNode == nullptr) {
        result.errorMessage = Utf8String("Invalid glTF skin node");
        return result;
    }
    if (skinNode->mesh == nullptr || skinNode->skin == nullptr) {
        result.errorMessage = Utf8String("glTF node has no mesh or skin");
        return result;
    }

    cgltf_skin* skin = skinNode->skin;
    const std::uint32_t jointCount = static_cast<std::uint32_t>(skin->joints_count);
    if (jointCount == 0) {
        result.errorMessage = Utf8String("glTF skin has no joints");
        return result;
    }
    if (jointCount > Skeleton::MaxJoints) {
        result.errorMessage = Utf8String("glTF skin joint count exceeds Skeleton::MaxJoints (128)");
        return result;
    }

    cgltf_float wm[16]{};
    cgltf_node_transform_world(skinNode, wm);
    const Matrix4 bakeWorld = Matrix4FromCgltf(wm);
    Matrix4 invBake{};
    if (!bakeWorld.TryInvert(invBake)) {
        invBake = Matrix4::Identity;
    }

    auto skeleton = MakeShared<Skeleton>();
    skeleton->jointCount = jointCount;
    skeleton->jointParents.Resize(jointCount);
    skeleton->inverseBind.Resize(jointCount);
    skeleton->restLocal.Resize(jointCount);
    skeleton->jointNames.Resize(jointCount);

    for (std::uint32_t i = 0; i < jointCount; ++i) {
        cgltf_node* ni = skin->joints[i];
        skeleton->jointNames[i] = Utf8String(ni->name != nullptr ? ni->name : "");
        std::int32_t parentIdx = -1;
        for (cgltf_node* p = ni->parent; p != nullptr; p = p->parent) {
            const std::uint32_t pj = JointIndexForNode(skin, p);
            if (pj != ~0u) {
                parentIdx = static_cast<std::int32_t>(pj);
                break;
            }
        }
        skeleton->jointParents[i] = parentIdx;
        skeleton->restLocal[i] = NodeLocalTransform(ni);

        Matrix4 ibm = Matrix4::Identity;
        if (skin->inverse_bind_matrices != nullptr) {
            float m16[16]{};
            if (cgltf_accessor_read_float(skin->inverse_bind_matrices, static_cast<cgltf_size>(i), m16, 16)) {
                for (int k = 0; k < 16; ++k) {
                    ibm.m[k] = m16[k];
                }
            }
        }
        skeleton->inverseBind[i] = ibm * invBake;
    }

    {
        Array<Matrix4> restLocalM;
        restLocalM.Resize(jointCount);
        for (std::uint32_t ji = 0; ji < jointCount; ++ji) {
            restLocalM[ji] = skeleton->restLocal[ji].ToMatrix4();
        }
        Array<Matrix4> partialBindWorld;
        Skeleton::ComputeJointWorldMatrices(jointCount, skeleton->jointParents, restLocalM, partialBindWorld);
        skeleton->jointGlobalPrefix.Resize(jointCount);
        for (std::uint32_t ji = 0; ji < jointCount; ++ji) {
            cgltf_float fullWm[16]{};
            cgltf_node_transform_world(skin->joints[ji], fullWm);
            const Matrix4 fullBind = Matrix4FromCgltf(fullWm);
            Matrix4 invPartial{};
            if (!partialBindWorld[ji].TryInvert(invPartial)) {
                invPartial = Matrix4::Identity;
            }
            skeleton->jointGlobalPrefix[ji] = fullBind * invPartial;
        }
    }

    auto mesh = MakeShared<SkinnedMesh>(Utf8String(sourcePath != nullptr ? sourcePath : "gltf_skinned_mesh"));
    mesh->Clear();

    const cgltf_mesh* cmesh = skinNode->mesh;
    bool meshHadNormals = false;
    bool meshHadTangents = false;
    for (cgltf_size pi = 0; pi < cmesh->primitives_count; ++pi) {
        const cgltf_primitive& prim = cmesh->primitives[pi];
        const std::uint32_t tc = BaseColorTexCoordSet(&prim);
        bool primHadNormals = false;
        bool primHadTangents = false;
        const GltfMeshBuildOutcome primitiveOutcome = GltfSkinnedMeshBuilder::AppendSkinnedPrimitive(
                data, &prim, bakeWorld, tc, &primHadNormals, &primHadTangents, *mesh);
        if (!primitiveOutcome.ok) {
            result.errorMessage = primitiveOutcome.errorMessage.IsEmpty() ? Utf8String("Failed to build skinned primitive") :
                                                                              primitiveOutcome.errorMessage;
            return result;
        }
        meshHadNormals = meshHadNormals || primHadNormals;
        meshHadTangents = meshHadTangents || primHadTangents;
    }

    if (mesh->GetVertices().IsEmpty()) {
        result.errorMessage = Utf8String("Skinned mesh has no vertices");
        return result;
    }

    const bool flipWinding = ShouldFlipGltfTriangleWinding(bakeWorld);
    if (!meshHadNormals) {
        SkinnedMesh::RecomputeSmoothNormals(*mesh);
        if (flipWinding) {
            Array<SkinnedMesh::Vertex>& verts = mesh->GetVertices();
            for (std::size_t vi = 0; vi < verts.GetSize(); ++vi) {
                verts[vi].normal = -verts[vi].normal;
            }
        }
    }
    if (!meshHadTangents) {
        SkinnedMesh::RecomputeTangentSpace(*mesh);
    }

    for (cgltf_size ai = 0; ai < data->animations_count; ++ai) {
        const cgltf_animation& anim = data->animations[ai];
        Skeleton::AnimationClip clip{};

        float maxT = 0.0F;
        for (cgltf_size ci = 0; ci < anim.channels_count; ++ci) {
            const cgltf_animation_channel& ch = anim.channels[ci];
            if (ch.target_node == nullptr || ch.sampler == nullptr) {
                continue;
            }
            const std::uint32_t ji = JointIndexForNode(skin, ch.target_node);
            if (ji == ~0u || ji >= jointCount) {
                continue;
            }
            const cgltf_animation_sampler& samp = *ch.sampler;
            if (samp.input == nullptr || samp.output == nullptr) {
                continue;
            }
            const cgltf_size keyCount = samp.input->count;
            if (keyCount == 0) {
                continue;
            }
            Array<float> times;
            times.Resize(static_cast<std::size_t>(keyCount));
            for (cgltf_size k = 0; k < keyCount; ++k) {
                float tv = 0.0F;
                cgltf_accessor_read_float(samp.input, k, &tv, 1);
                times[static_cast<std::size_t>(k)] = tv;
                if (tv > maxT) {
                    maxT = tv;
                }
            }

            if (ch.target_path == cgltf_animation_path_type_translation) {
                Skeleton::Vec3Channel vc{};
                vc.jointIndex = ji;
                vc.times = MoveTemp(times);
                vc.values.Resize(static_cast<std::size_t>(keyCount));
                for (cgltf_size k = 0; k < keyCount; ++k) {
                    float v3[3]{};
                    cgltf_accessor_read_float(samp.output, k, v3, 3);
                    vc.values[static_cast<std::size_t>(k)] = {v3[0], v3[1], v3[2]};
                }
                clip.translations.PushBack(MoveTemp(vc));
            } else if (ch.target_path == cgltf_animation_path_type_rotation) {
                Skeleton::QuatChannel qc{};
                qc.jointIndex = ji;
                qc.times = MoveTemp(times);
                qc.values.Resize(static_cast<std::size_t>(keyCount));
                for (cgltf_size k = 0; k < keyCount; ++k) {
                    float v4[4]{};
                    cgltf_accessor_read_float(samp.output, k, v4, 4);
                    qc.values[static_cast<std::size_t>(k)] = {v4[0], v4[1], v4[2], v4[3]};
                }
                clip.rotations.PushBack(MoveTemp(qc));
            } else if (ch.target_path == cgltf_animation_path_type_scale) {
                Skeleton::Vec3Channel vc{};
                vc.jointIndex = ji;
                vc.times = MoveTemp(times);
                vc.values.Resize(static_cast<std::size_t>(keyCount));
                for (cgltf_size k = 0; k < keyCount; ++k) {
                    float v3[3]{};
                    cgltf_accessor_read_float(samp.output, k, v3, 3);
                    vc.values[static_cast<std::size_t>(k)] = {v3[0], v3[1], v3[2]};
                }
                clip.scales.PushBack(MoveTemp(vc));
            }
        }

        clip.duration = maxT > 0.0F ? maxT : 1.0e-4F;
        skeleton->clips.PushBack(MoveTemp(clip));
        const char* nm = anim.name != nullptr ? anim.name : "";
        skeleton->clipNames.PushBack(Utf8String(nm));
    }

    result.ok = true;
    result.mesh = mesh;
    result.skeleton = skeleton;
    return result;
}

}  // namespace Spark
