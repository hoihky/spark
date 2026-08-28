#include "spark/scene/prefab/GltfPrefabImporter.hpp"

#include "spark/config.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Transform.hpp"
#include "spark/scene/assets/GltfAssetPathResolver.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/assets/gltf/GltfSceneDocument.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace Spark {

namespace {

Transform ComputeNodeWorldTransform(const GltfSceneDocument& document, const std::uint32_t nodeIndex) noexcept {
    Matrix4 worldMatrix = Matrix4::Identity;
    std::uint32_t current = nodeIndex;
    Array<std::uint32_t> chain{};
    while (current < document.nodes.GetSize()) {
        chain.PushBack(current);
        const std::int32_t parentIndex = document.nodes[current].parentIndex;
        if (parentIndex < 0) {
            break;
        }
        current = static_cast<std::uint32_t>(parentIndex);
    }
    for (std::size_t i = chain.GetSize(); i > 0; --i) {
        worldMatrix = worldMatrix * document.nodes[chain[i - 1]].localTransform.ToMatrix4();
    }
    return Transform::FromAffineMatrix(worldMatrix);
}

bool TryComputeSceneBounds(const GltfSceneDocument& document, Vector3& outMin, Vector3& outMax) noexcept {
    bool found = false;
    outMin = Vector3::Zero;
    outMax = Vector3::Zero;
    for (std::size_t nodeIndex = 0; nodeIndex < document.nodes.GetSize(); ++nodeIndex) {
        const GltfSceneNode& node = document.nodes[nodeIndex];
        SharedPtr<Mesh> mesh;
        if (node.HasMesh() && node.meshIndex < document.meshes.GetSize()) {
            mesh = document.meshes[node.meshIndex];
        }
        if (!mesh) {
            continue;
        }
        Vector3 localMin{};
        Vector3 localMax{};
        if (!mesh->TryComputeAxisAlignedBounds(localMin, localMax)) {
            continue;
        }
        const Transform world = ComputeNodeWorldTransform(document, static_cast<std::uint32_t>(nodeIndex));
        const Vector3 corners[8] = {
                world.TransformPosition({localMin.x, localMin.y, localMin.z}),
                world.TransformPosition({localMax.x, localMin.y, localMin.z}),
                world.TransformPosition({localMin.x, localMax.y, localMin.z}),
                world.TransformPosition({localMax.x, localMax.y, localMin.z}),
                world.TransformPosition({localMin.x, localMin.y, localMax.z}),
                world.TransformPosition({localMax.x, localMin.y, localMax.z}),
                world.TransformPosition({localMin.x, localMax.y, localMax.z}),
                world.TransformPosition({localMax.x, localMax.y, localMax.z}),
        };
        for (const Vector3& corner : corners) {
            if (!found) {
                outMin = corner;
                outMax = corner;
                found = true;
            } else {
                outMin.x = std::min(outMin.x, corner.x);
                outMin.y = std::min(outMin.y, corner.y);
                outMin.z = std::min(outMin.z, corner.z);
                outMax.x = std::max(outMax.x, corner.x);
                outMax.y = std::max(outMax.y, corner.y);
                outMax.z = std::max(outMax.z, corner.z);
            }
        }
    }
    return found;
}

void EnsurePrefabOutputDir(const char* subdir) noexcept {
    char sourceDir[1024]{};
    char buildDir[1024]{};
    std::snprintf(sourceDir, sizeof(sourceDir), "%s/%s", SPARK_ASSETS_DIR, subdir != nullptr ? subdir : "prefabs");
    std::snprintf(buildDir, sizeof(buildDir), "%s/%s", SPARK_BUILD_ASSETS_DIR, subdir != nullptr ? subdir : "prefabs");
    struct stat st {};
    if (stat(sourceDir, &st) != 0) {
        (void)mkdir(sourceDir, 0755);
    }
    if (stat(buildDir, &st) != 0) {
        (void)mkdir(buildDir, 0755);
    }
}

SceneDocument BuildPrefabDocument(
        const Utf8String& displayName,
        const char* assetsRoot,
        const char* gltfAssetRel,
        const float uniformScale,
        const float groundYOffset) noexcept {
    SceneDocument document{};
    document.header.name = displayName;
    document.header.assetsRoot = Utf8String(assetsRoot != nullptr ? assetsRoot : "assets");

    EntityRecord entity{};
    entity.id = 1;
    entity.parentId = -1;
    entity.name = displayName;

    ComponentRecord transform{};
    transform.kind = Utf8String("transform");
    char transformPayload[256]{};
    std::snprintf(
            transformPayload,
            sizeof(transformPayload),
            "%.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f",
            uniformScale,
            uniformScale,
            uniformScale,
            0.0F,
            groundYOffset,
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            1.0F);
    transform.payload = Utf8String(transformPayload);

    ComponentRecord gltfScene{};
    gltfScene.kind = Utf8String("gltf_scene");
    char gltfPayload[512]{};
    std::snprintf(gltfPayload, sizeof(gltfPayload), "\"%s\"", gltfAssetRel);
    gltfScene.payload = Utf8String(gltfPayload);

    entity.components.PushBack(MoveTemp(transform));
    entity.components.PushBack(MoveTemp(gltfScene));
    document.entities.PushBack(MoveTemp(entity));
    return document;
}

}  // namespace

GltfPrefabImportResult GltfPrefabImporter::ImportFromGltf(
        GameWorld& world,
        const char* gltfPath,
        const GltfPrefabImportOptions& options) const {
    GltfPrefabImportResult result{};
    if (gltfPath == nullptr || gltfPath[0] == '\0') {
        result.errorMessage = Utf8String("Empty glTF path.");
        return result;
    }

    Utf8String stagingError{};
    if (!GltfAssetPathResolver::EnsureInAssetLibrary(gltfPath, result.gltfAssetRel, &stagingError)) {
        result.errorMessage = stagingError.IsEmpty() ? Utf8String("Could not stage glTF in assets.") : MoveTemp(stagingError);
        return result;
    }

    const Utf8String readablePath = ScenePathResolver::ResolveReadablePath(result.gltfAssetRel.CStr());
    if (readablePath.IsEmpty()) {
        result.errorMessage = Utf8String("Staged glTF is not readable.");
        return result;
    }

    const AssetLoadOutcome<GltfSceneDocument> loaded = world.TryLoadGltfScene(readablePath.CStr());
    if (!loaded.ok || loaded.value.IsEmpty()) {
        result.errorMessage = loaded.errorMessage.IsEmpty() ? Utf8String("Could not parse glTF scene.") : loaded.errorMessage;
        return result;
    }

    Utf8String baseName = options.prefabBaseName;
    if (baseName.IsEmpty()) {
        baseName = GltfAssetPathResolver::SanitizeBaseName(gltfPath);
    }
    Utf8String prefabBase{};
    prefabBase.AppendUtf8("imported_");
    prefabBase.AppendUtf8(baseName);
    result.prefabFileName = prefabBase;
    result.prefabFileName.AppendUtf8(".sparkscene");

    const char* outputSubdir = options.outputSubdir != nullptr ? options.outputSubdir : "prefabs";
    result.prefabCaptureHint = Utf8String(outputSubdir);
    result.prefabCaptureHint.AppendUtf8("/");
    result.prefabCaptureHint.AppendUtf8(result.prefabFileName);

    float uniformScale = 1.0F;
    float groundYOffset = 0.0F;
    Vector3 boundsMin{};
    Vector3 boundsMax{};
    if (TryComputeSceneBounds(loaded.value, boundsMin, boundsMax)) {
        const float dx = boundsMax.x - boundsMin.x;
        const float dy = boundsMax.y - boundsMin.y;
        const float dz = boundsMax.z - boundsMin.z;
        const float maxExtent = std::max({dx, dy, dz});
        if (maxExtent > 1.0e-4F && options.targetMaxExtent > 1.0e-4F) {
            uniformScale = options.targetMaxExtent / maxExtent;
        }
        constexpr float kGroundClearance = 0.02F;
        groundYOffset = -boundsMin.y * uniformScale + kGroundClearance;
    }

    const SceneDocument document = BuildPrefabDocument(
            baseName,
            options.assetsRoot,
            result.gltfAssetRel.CStr(),
            uniformScale,
            groundYOffset);

    EnsurePrefabOutputDir(outputSubdir);
    SceneSerializer serializer{};
    const Utf8String sourcePrefabPath = ScenePathResolver::SourceAssetPath(outputSubdir, result.prefabFileName.CStr());
    const Utf8String buildPrefabPath = ScenePathResolver::BuildRuntimePath(outputSubdir, result.prefabFileName.CStr());
    if (!serializer.WriteToFile(document, sourcePrefabPath.CStr()) &&
        !serializer.WriteToFile(document, buildPrefabPath.CStr())) {
        result.errorMessage = Utf8String("Failed to write prefab file.");
        return result;
    }
    (void)serializer.WriteToFile(document, buildPrefabPath.CStr());

    Utf8String menuLabel{};
    menuLabel.AppendUtf8("Prefab — ");
    menuLabel.AppendUtf8(baseName);
    result.catalogEntry = PrefabCatalogEntry{
            .menuLabel = MoveTemp(menuLabel),
            .fileName = result.prefabFileName,
            .captureMeshHint = result.prefabCaptureHint,
    };
    result.ok = true;
    return result;
}

}  // namespace Spark
