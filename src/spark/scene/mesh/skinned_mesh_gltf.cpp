#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/assets/gltf/GltfDataLoader.hpp"
#include "spark/scene/assets/gltf/GltfSkinNodeLoader.hpp"
#include "spark/scene/texture/Texture2D.hpp"

#include "cgltf.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace Spark {

namespace {

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
        Array<GltfMaterialDesc>* outMaterials,
        Utf8String* outError) {
    auto setError = [&](const char* msg) {
        if (outError != nullptr) {
            *outError = Utf8String(msg);
        }
    };
    if (path == nullptr || path[0] == '\0') {
        setError("Empty skinned glTF path");
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

    const GltfDataLoadResult loaded = LoadParsedGltfFile(path);
    if (!loaded.ok || loaded.data == nullptr) {
        setError(loaded.errorMessage.IsEmpty() ? "Failed to load skinned glTF file" : loaded.errorMessage.CStr());
        return false;
    }
    cgltf_data* data = loaded.data;

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
        setError("No skinned mesh node found in glTF");
        return false;
    }

    const GltfSkinNodeBuildResult built = TryBuildSkinNode(data, skinNode, path);
    if (!built.ok) {
        cgltf_free(data);
        setError(built.errorMessage.IsEmpty() ? "Failed to build skinned glTF node" : built.errorMessage.CStr());
        return false;
    }

    outMesh = MoveTemp(*built.mesh);
    outSkeleton = MoveTemp(*built.skeleton);

    cgltf_float wm[16]{};
    cgltf_node_transform_world(skinNode, wm);
    const Matrix4 bakeWorld = Matrix4FromCgltf(wm);
    const Quaternion bindUpLocal = BindUpFromSkinNodeBakeWorld(bakeWorld);

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
                constexpr float kTargetX = 0.0F;
                constexpr float kTargetZ = -1.0F;
                const float crossY = hx * kTargetZ - hz * kTargetX;
                const float dotH = hx * kTargetX + hz * kTargetZ;
                *outBindFacingYawOffset = -std::atan2(crossY, dotH);
            }
        }
    }

    const cgltf_mesh* cmesh = skinNode->mesh;
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
