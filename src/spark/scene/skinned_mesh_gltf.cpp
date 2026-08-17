#include "spark/scene/GltfMaterial.hpp"
#include "spark/scene/SkinnedMesh.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Texture2D.hpp"

#include "cgltf.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

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

void AppendSkinnedPrimitive(
        const cgltf_data* data,
        const cgltf_primitive* prim,
        const Matrix4& bakeWorld,
        std::uint32_t texCoordSet,
        bool* outHadNormals,
        bool* outHadTangents,
        SkinnedMesh& outMesh) {
    if (prim == nullptr || prim->type != cgltf_primitive_type_triangles) {
        return;
    }
    const cgltf_accessor* pos = nullptr;
    const cgltf_accessor* nrm = nullptr;
    const cgltf_accessor* tan = nullptr;
    const cgltf_accessor* uv = nullptr;
    const cgltf_accessor* joints = nullptr;
    const cgltf_accessor* weights = nullptr;
    for (cgltf_size ai = 0; ai < prim->attributes_count; ++ai) {
        const cgltf_attribute& a = prim->attributes[ai];
        if (a.type == cgltf_attribute_type_position) {
            pos = a.data;
        } else if (a.type == cgltf_attribute_type_normal) {
            nrm = a.data;
        } else if (a.type == cgltf_attribute_type_tangent) {
            tan = a.data;
        } else if (a.type == cgltf_attribute_type_texcoord &&
                   static_cast<std::uint32_t>(a.index) == texCoordSet) {
            uv = a.data;
        } else if (a.type == cgltf_attribute_type_joints && a.index == 0) {
            joints = a.data;
        } else if (a.type == cgltf_attribute_type_weights && a.index == 0) {
            weights = a.data;
        }
    }
    if (uv == nullptr && texCoordSet != 0) {
        for (cgltf_size ai = 0; ai < prim->attributes_count; ++ai) {
            const cgltf_attribute& a = prim->attributes[ai];
            if (a.type == cgltf_attribute_type_texcoord && a.index == 0) {
                uv = a.data;
                break;
            }
        }
    }
    if (pos == nullptr || joints == nullptr || weights == nullptr) {
        return;
    }

    const std::uint32_t indexOffset = static_cast<std::uint32_t>(outMesh.GetIndices().GetSize());
    if (outHadNormals != nullptr) {
        *outHadNormals = *outHadNormals || (nrm != nullptr && nrm->type == cgltf_type_vec3);
    }
    if (outHadTangents != nullptr) {
        *outHadTangents = *outHadTangents || (tan != nullptr && tan->type == cgltf_type_vec4);
    }

    const cgltf_size vcount = pos->count;
    const std::uint32_t base = static_cast<std::uint32_t>(outMesh.GetVertices().GetSize());
    const bool flipWinding = ShouldFlipGltfTriangleWinding(bakeWorld);

    for (cgltf_size vi = 0; vi < vcount; ++vi) {
        float p[3]{};
        cgltf_accessor_read_float(pos, vi, p, 3);
        Vector3 pw = bakeWorld.TransformPoint({p[0], p[1], p[2]});
        Vector3 nw{0.0F, 1.0F, 0.0F};
        if (nrm != nullptr && nrm->type == cgltf_type_vec3) {
            float n[3]{};
            cgltf_accessor_read_float(nrm, vi, n, 3);
            nw = bakeWorld.TransformVector({n[0], n[1], n[2]}).Normalized();
        }
        Vector2 tc{0.0F, 0.0F};
        if (uv != nullptr && uv->type == cgltf_type_vec2) {
            float t2[2]{};
            cgltf_accessor_read_float(uv, vi, t2, 2);
            tc = {t2[0], t2[1]};
        }
        Vector4 tangent{};
        if (tan != nullptr && tan->type == cgltf_type_vec4) {
            float t4[4]{};
            cgltf_accessor_read_float(tan, vi, t4, 4);
            const Vector3 tw = bakeWorld.TransformVector({t4[0], t4[1], t4[2]}).Normalized();
            tangent = {tw.x, tw.y, tw.z, t4[3]};
        }
        cgltf_uint ji[4]{};
        if (!cgltf_accessor_read_uint(joints, vi, ji, 4)) {
            ji[0] = ji[1] = ji[2] = ji[3] = 0;
        }
        float wf[4]{1.0F, 0.0F, 0.0F, 0.0F};
        cgltf_accessor_read_float(weights, vi, wf, 4);

        float wsum = wf[0] + wf[1] + wf[2] + wf[3];
        if (wsum > 1.0e-6F) {
            const float inv = 1.0F / wsum;
            wf[0] *= inv;
            wf[1] *= inv;
            wf[2] *= inv;
            wf[3] *= inv;
        } else {
            wf[0] = 1.0F;
            wf[1] = wf[2] = wf[3] = 0.0F;
        }

        SkinnedMesh::Vertex sv{};
        sv.position = pw;
        sv.normal = nw;
        sv.texCoord = tc;
        sv.tangent = tangent;
        sv.joints[0] = static_cast<std::uint32_t>(ji[0]);
        sv.joints[1] = static_cast<std::uint32_t>(ji[1]);
        sv.joints[2] = static_cast<std::uint32_t>(ji[2]);
        sv.joints[3] = static_cast<std::uint32_t>(ji[3]);
        sv.weights[0] = wf[0];
        sv.weights[1] = wf[1];
        sv.weights[2] = wf[2];
        sv.weights[3] = wf[3];
        outMesh.GetVertices().PushBack(sv);
    }

    if (prim->indices != nullptr) {
        const cgltf_accessor* idx = prim->indices;
        const cgltf_size icount = idx->count;
        if (icount % 3 != 0) {
            return;
        }
        for (cgltf_size ti = 0; ti < icount; ti += 3) {
            const std::uint32_t i0 = static_cast<std::uint32_t>(cgltf_accessor_read_index(idx, ti + 0));
            const std::uint32_t i1 = static_cast<std::uint32_t>(cgltf_accessor_read_index(idx, ti + 1));
            const std::uint32_t i2 = static_cast<std::uint32_t>(cgltf_accessor_read_index(idx, ti + 2));
            if (flipWinding) {
                outMesh.AddTriangle(base + i0, base + i2, base + i1);
            } else {
                outMesh.AddTriangle(base + i0, base + i1, base + i2);
            }
        }
    } else {
        for (cgltf_size ti = 0; ti + 2 < vcount; ti += 3) {
            const std::uint32_t i0 = base + static_cast<std::uint32_t>(ti);
            const std::uint32_t i1 = base + static_cast<std::uint32_t>(ti + 1);
            const std::uint32_t i2 = base + static_cast<std::uint32_t>(ti + 2);
            if (flipWinding) {
                outMesh.AddTriangle(i0, i2, i1);
            } else {
                outMesh.AddTriangle(i0, i1, i2);
            }
        }
    }

    const std::uint32_t indexCount =
            static_cast<std::uint32_t>(outMesh.GetIndices().GetSize()) - indexOffset;
    if (indexCount == 0) {
        return;
    }
    MeshSubmesh submesh{};
    submesh.indexOffset = indexOffset;
    submesh.indexCount = indexCount;
    if (data != nullptr && prim->material != nullptr && data->materials_count > 0) {
        const std::ptrdiff_t offset = prim->material - data->materials;
        if (offset >= 0 && static_cast<cgltf_size>(offset) < data->materials_count) {
            submesh.materialIndex = static_cast<std::uint32_t>(offset);
        }
    }
    outMesh.GetSubmeshes().PushBack(submesh);
}

void ScanForSkinnedMeshNode(cgltf_node* node, cgltf_node** outSkinNode) {
    if (node == nullptr || outSkinNode == nullptr || *outSkinNode != nullptr) {
        return;
    }
    if (node->mesh != nullptr && node->skin != nullptr) {
        *outSkinNode = node;
        return;
    }
    for (cgltf_size c = 0; c < node->children_count; ++c) {
        ScanForSkinnedMeshNode(node->children[c], outSkinNode);
    }
}

bool NameContainsWalk(const char* name) {
    if (name == nullptr) {
        return false;
    }
    const char* p = name;
    while (*p != '\0') {
        const char* w = "walk";
        const char* q = p;
        bool match = true;
        for (std::size_t k = 0; w[k] != '\0'; ++k) {
            const char c = q[static_cast<std::size_t>(k)];
            if (c == '\0') {
                match = false;
                break;
            }
            if (std::tolower(static_cast<unsigned char>(c)) != static_cast<unsigned char>(w[k])) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
        ++p;
    }
    return false;
}

/** cgltf / glTF matrices are column-major; Spark::Matrix4 matches (same as mesh_gltf.cpp). */
Matrix4 Matrix4FromCgltf(const cgltf_float* cm) noexcept {
    Matrix4 out{};
    for (int i = 0; i < 16; ++i) {
        out.m[i] = static_cast<float>(cm[i]);
    }
    return out;
}

/**
 * glTF skin nodes often sit under Z_UP / armature nodes so **mesh +Y is not character up** in world space.
 * Pick the local ±axis whose image under bakeWorld best aligns with world +Y, then shortest-arc to +Y.
 */
Quaternion BindUpFromSkinNodeBakeWorld(const Matrix4& bakeWorld) noexcept {
    static const Vector3 kAxes[6] = {Vector3::UnitX,  -Vector3::UnitX, Vector3::UnitY,  -Vector3::UnitY,
                                     Vector3::UnitZ,  -Vector3::UnitZ};
    float bestDot = -2.0F;
    Vector3 bestWorldDir = Vector3::UnitY;
    for (const Vector3& ax : kAxes) {
        Vector3 w = bakeWorld.TransformVector(ax);
        const float len2 = w.LengthSquared();
        if (len2 < 1.0e-20F) {
            continue;
        }
        w = w * (1.0F / std::sqrt(len2));
        const float d = Vector3::Dot(w, Vector3::UnitY);
        if (d > bestDot) {
            bestDot = d;
            bestWorldDir = w;
        }
    }
    if (bestDot > 1.0F - 1.0e-5F) {
        return Quaternion::Identity;
    }
    return Quaternion::FromShortestArc(bestWorldDir, Vector3::UnitY);
}

/**
 * Mesh-axis direction in baked space to use as "forward" for bindFacingYawOffset.
 * Prefer an axis whose horizontal (XZ) direction best matches CharacterCameraRig walk forward at camera yaw 0:
 * flatF = (0, 0, -1) → horizontal (0, -1) in (x, z). CesiumMan maps mesh -X to (0, 0, -1); mesh +Y maps to +X
 * (90° off), so "largest horizontal extent" alone picks the wrong axis.
 */
Vector3 ForwardHintFromBakeWorld(const Matrix4& bakeWorld) noexcept {
    static const Vector3 kAxes[6] = {Vector3::UnitX,  -Vector3::UnitX, Vector3::UnitY,  -Vector3::UnitY,
                                     Vector3::UnitZ,  -Vector3::UnitZ};
    constexpr float kWalkX = 0.0F;
    constexpr float kWalkZ = -1.0F;

    Vector3 best = bakeWorld.TransformVector(-Vector3::UnitZ);
    float bestAlign = -2.0F;
    const float bl2 = best.LengthSquared();
    if (bl2 >= 1.0e-20F) {
        best = best * (1.0F / std::sqrt(bl2));
        const float hx = best.x;
        const float hz = best.z;
        const float hLen2 = hx * hx + hz * hz;
        if (hLen2 >= 1.0e-12F) {
            const float invH = 1.0F / std::sqrt(hLen2);
            bestAlign = (hx * invH) * kWalkX + (hz * invH) * kWalkZ;
        }
    }
    for (const Vector3& ax : kAxes) {
        Vector3 w = bakeWorld.TransformVector(ax);
        const float len2 = w.LengthSquared();
        if (len2 < 1.0e-20F) {
            continue;
        }
        w = w * (1.0F / std::sqrt(len2));
        const float wx = w.x;
        const float wz = w.z;
        const float hLen2 = wx * wx + wz * wz;
        if (hLen2 < 1.0e-12F) {
            continue;
        }
        const float invH = 1.0F / std::sqrt(hLen2);
        const float align = (wx * invH) * kWalkX + (wz * invH) * kWalkZ;
        if (align > bestAlign) {
            bestAlign = align;
            best = w;
        }
    }
    if (bestAlign < -0.9F) {
        return Vector3{0.0F, 0.0F, -1.0F};
    }
    return best;
}

/**
 * Baked bind-pose vertices may be longest on X (body length) or Z while +Y is still the plausible up axis
 * (quadrupeds, forward-facing rigs). Only remap when +Y is the **thinnest** extent — the mesh is lying flat.
 */
Quaternion BindUpFromBakedVertexAabb(const Vector3& bmin, const Vector3& bmax) noexcept {
    const float ex = bmax.x - bmin.x;
    const float ey = bmax.y - bmin.y;
    const float ez = bmax.z - bmin.z;
    const float m = std::max({ex, ey, ez});
    if (m < 1.0e-6F) {
        return Quaternion::Identity;
    }
    if (ey >= ex * 0.72F && ey >= ez * 0.72F) {
        return Quaternion::Identity;
    }
    if (ey <= ex * 0.55F && ey <= ez * 0.55F) {
        if (ez >= ex * 0.85F) {
            return Quaternion::FromAxisAngle(Vector3::UnitX, -HalfPi);
        }
        if (ex >= ez * 0.85F) {
            return Quaternion::FromAxisAngle(Vector3::UnitZ, HalfPi);
        }
    }
    return Quaternion::Identity;
}

}  // namespace

bool TryLoadSkinnedCharacterFromGltf(
        const char* path,
        SkinnedMesh& outMesh,
        Skeleton& outSkeleton,
        SharedPtr<Texture2D>* outBaseColor,
        GltfMaterialDesc* outMaterial,
        std::uint32_t* outWalkClipIndex,
        Quaternion* outBindUpAlignment,
        float* outBindFacingYawOffset,
        Array<GltfMaterialDesc>* outMaterials) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (outWalkClipIndex != nullptr) {
        *outWalkClipIndex = 0;
    }
    if (outBindUpAlignment != nullptr) {
        *outBindUpAlignment = Quaternion::Identity;
    }
    if (outBindFacingYawOffset != nullptr) {
        *outBindFacingYawOffset = 0.0F;
    }

    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path, &data) != cgltf_result_success || data == nullptr) {
        return false;
    }
    if (cgltf_load_buffers(&options, data, path) != cgltf_result_success) {
        cgltf_free(data);
        return false;
    }

    outMesh.Clear();
    outMesh.GetName() = Utf8String(path);
    outSkeleton.jointCount = 0;
    outSkeleton.jointParents.Clear();
    outSkeleton.inverseBind.Clear();
    outSkeleton.restLocal.Clear();
    outSkeleton.clips.Clear();
    outSkeleton.clipNames.Clear();
    outSkeleton.jointGlobalPrefix.Clear();

    cgltf_scene* primary = data->scene;
    if (primary == nullptr && data->scenes_count > 0) {
        primary = &data->scenes[0];
    }

    cgltf_node* skinNode = nullptr;
    if (primary != nullptr) {
        for (cgltf_size i = 0; i < primary->nodes_count; ++i) {
            ScanForSkinnedMeshNode(primary->nodes[i], &skinNode);
            if (skinNode != nullptr) {
                break;
            }
        }
    }
    if (skinNode == nullptr) {
        for (cgltf_size si = 0; si < data->nodes_count; ++si) {
            ScanForSkinnedMeshNode(&data->nodes[si], &skinNode);
            if (skinNode != nullptr) {
                break;
            }
        }
    }

    if (skinNode == nullptr || skinNode->mesh == nullptr || skinNode->skin == nullptr) {
        cgltf_free(data);
        return false;
    }

    cgltf_skin* skin = skinNode->skin;
    const std::uint32_t jointCount = static_cast<std::uint32_t>(skin->joints_count);
    if (jointCount == 0 || jointCount > Skeleton::MaxJoints) {
        cgltf_free(data);
        return false;
    }

    cgltf_float wm[16]{};
    cgltf_node_transform_world(skinNode, wm);
    const Matrix4 bakeWorld = Matrix4FromCgltf(wm);
    const Quaternion bindUpLocal = BindUpFromSkinNodeBakeWorld(bakeWorld);
    Matrix4 invBake{};
    if (!bakeWorld.TryInvert(invBake)) {
        invBake = Matrix4::Identity;
    }

    outSkeleton.jointCount = jointCount;
    outSkeleton.jointParents.Resize(jointCount);
    outSkeleton.inverseBind.Resize(jointCount);
    outSkeleton.restLocal.Resize(jointCount);

    for (std::uint32_t i = 0; i < jointCount; ++i) {
        cgltf_node* ni = skin->joints[i];
        std::int32_t parentIdx = -1;
        for (cgltf_node* p = ni->parent; p != nullptr; p = p->parent) {
            const std::uint32_t pj = JointIndexForNode(skin, p);
            if (pj != ~0u) {
                parentIdx = static_cast<std::int32_t>(pj);
                break;
            }
        }
        outSkeleton.jointParents[i] = parentIdx;
        outSkeleton.restLocal[i] = NodeLocalTransform(ni);

        Matrix4 ibm = Matrix4::Identity;
        if (skin->inverse_bind_matrices != nullptr) {
            float m16[16]{};
            if (cgltf_accessor_read_float(skin->inverse_bind_matrices, static_cast<cgltf_size>(i), m16, 16)) {
                for (int k = 0; k < 16; ++k) {
                    ibm.m[k] = m16[k];
                }
            }
        }
        outSkeleton.inverseBind[i] = ibm * invBake;
    }

    {
        Array<Matrix4> restLocalM;
        restLocalM.Resize(jointCount);
        for (std::uint32_t ji = 0; ji < jointCount; ++ji) {
            restLocalM[ji] = outSkeleton.restLocal[ji].ToMatrix4();
        }
        Array<Matrix4> partialBindWorld;
        Skeleton::ComputeJointWorldMatrices(jointCount, outSkeleton.jointParents, restLocalM, partialBindWorld);
        outSkeleton.jointGlobalPrefix.Resize(jointCount);
        for (std::uint32_t ji = 0; ji < jointCount; ++ji) {
            cgltf_float fullWm[16]{};
            cgltf_node_transform_world(skin->joints[ji], fullWm);
            const Matrix4 fullBind = Matrix4FromCgltf(fullWm);
            Matrix4 invPartial{};
            if (!partialBindWorld[ji].TryInvert(invPartial)) {
                invPartial = Matrix4::Identity;
            }
            outSkeleton.jointGlobalPrefix[ji] = fullBind * invPartial;
        }
    }

    const cgltf_mesh* cmesh = skinNode->mesh;
    bool meshHadNormals = false;
    bool meshHadTangents = false;
    for (cgltf_size pi = 0; pi < cmesh->primitives_count; ++pi) {
        const cgltf_primitive& prim = cmesh->primitives[pi];
        const std::uint32_t tc = BaseColorTexCoordSet(&prim);
        bool primHadNormals = false;
        bool primHadTangents = false;
        AppendSkinnedPrimitive(data, &prim, bakeWorld, tc, &primHadNormals, &primHadTangents, outMesh);
        meshHadNormals = meshHadNormals || primHadNormals;
        meshHadTangents = meshHadTangents || primHadTangents;
    }

    if (outMesh.GetVertices().IsEmpty()) {
        cgltf_free(data);
        return false;
    }
    // glTF file normals are outward for CCW faces; indices are flipped for Vulkan when det >= 0.
    // Recomputing from flipped (CW) indices inverts normals — keep authored normals when present.
    const bool flipWinding = ShouldFlipGltfTriangleWinding(bakeWorld);
    if (!meshHadNormals) {
        SkinnedMesh::RecomputeSmoothNormals(outMesh);
        if (flipWinding) {
            Array<SkinnedMesh::Vertex>& verts = outMesh.GetVertices();
            for (std::size_t vi = 0; vi < verts.GetSize(); ++vi) {
                verts[vi].normal = -verts[vi].normal;
            }
        }
    }
    if (!meshHadTangents) {
        SkinnedMesh::RecomputeTangentSpace(outMesh);
    }

    {
        Vector3 bmin{};
        Vector3 bmax{};
        {
            const Array<SkinnedMesh::Vertex>& sv = outMesh.GetVertices();
            bmin = sv[0].position;
            bmax = sv[0].position;
            for (std::size_t vi = 1; vi < sv.GetSize(); ++vi) {
                const Vector3& p = sv[vi].position;
                bmin.x = std::min(bmin.x, p.x);
                bmin.y = std::min(bmin.y, p.y);
                bmin.z = std::min(bmin.z, p.z);
                bmax.x = std::max(bmax.x, p.x);
                bmax.y = std::max(bmax.y, p.y);
                bmax.z = std::max(bmax.z, p.z);
            }
        }
        const Quaternion qAabb = BindUpFromBakedVertexAabb(bmin, bmax);
        const Quaternion bindUpCombined = (qAabb * bindUpLocal).Normalized();

        if (outBindUpAlignment != nullptr) {
            *outBindUpAlignment = bindUpCombined;
        }
        if (outBindFacingYawOffset != nullptr) {
            const Vector3 fBaked = ForwardHintFromBakeWorld(bakeWorld);
            const Vector3 fUpright = bindUpCombined.RotateVector(fBaked);
            const float hx = fUpright.x;
            const float hz = fUpright.z;
            const float hLen2 = hx * hx + hz * hz;
            if (hLen2 >= 1.0e-12F) {
                // Walk forward at camera yaw 0: CharacterCameraRig flatF = (0,0,-1); horizontal (tx,tz)=(0,-1).
                // Signed angle in XZ from mesh forward to that target (atan2(cross, dot)), then negate so
                // Quaternion::FromAxisAngle(UnitY, offset) matches Matrix4::Rotation (engine Y sign).
                constexpr float kTargetX = 0.0F;
                constexpr float kTargetZ = -1.0F;
                const float crossY = hx * kTargetZ - hz * kTargetX;
                const float dotH = hx * kTargetX + hz * kTargetZ;
                *outBindFacingYawOffset = -std::atan2(crossY, dotH);
            }
        }
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
        outSkeleton.clips.PushBack(MoveTemp(clip));
        const char* nm = anim.name != nullptr ? anim.name : "";
        outSkeleton.clipNames.PushBack(Utf8String(nm));
    }

    if (outMaterial != nullptr || outBaseColor != nullptr || outMaterials != nullptr) {
        Array<GltfMaterialDesc> loadedMaterials;
        GltfMaterialLoader::LoadAll(data, path, loadedMaterials);
        if (outMaterials != nullptr) {
            *outMaterials = loadedMaterials;
        }
        GltfMaterialDesc material{};
        if (!loadedMaterials.IsEmpty()) {
            material = loadedMaterials[0];
        } else {
            (void)TryLoadPrimaryGltfMaterial(data, cmesh, path, material);
        }
        if (outMaterial != nullptr) {
            *outMaterial = material;
        }
        if (outBaseColor != nullptr) {
            *outBaseColor = material.baseColor;
        }
    }

    if (outWalkClipIndex != nullptr && !outSkeleton.clipNames.IsEmpty()) {
        for (std::size_t i = 0; i < outSkeleton.clipNames.GetSize(); ++i) {
            if (NameContainsWalk(outSkeleton.clipNames[i].CStr())) {
                *outWalkClipIndex = static_cast<std::uint32_t>(i);
                break;
            }
        }
    }

    cgltf_free(data);
    return true;
}

}  // namespace Spark
