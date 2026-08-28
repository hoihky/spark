#include "spark/scene/editor/GltfImportService.hpp"

#include "spark/platform/NativeFilePicker.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/core/SceneManager.hpp"
#include "spark/scene/prefab/PrefabInstantiator.hpp"

namespace Spark {

void GltfImportService::Bind(
        PrefabCatalog* inCatalog,
        ScenePlacementActionRegistry* inPlacementActions) noexcept {
    catalog = inCatalog;
    placementActions = inPlacementActions;
}

GltfImportService::ImportResult GltfImportService::RegisterImportedPrefab(
        GameWorld& world,
        const GltfPrefabImportResult& importResult) const {
    ImportResult result{};
    if (!importResult.ok) {
        result.message = importResult.errorMessage.IsEmpty() ? Utf8String("glTF import failed.") : importResult.errorMessage;
        return result;
    }
    if (catalog == nullptr) {
        result.message = Utf8String("Prefab catalog is not bound.");
        return result;
    }

    catalog->Register(importResult.catalogEntry);
    if (placementActions != nullptr) {
        ScenePlacementActionRegistry::RegisterPrefabAction(*placementActions, importResult.catalogEntry);
    }

    (void)world;
    result.ok = true;
    result.catalogEntry = importResult.catalogEntry;
    result.message = Utf8String("Imported glTF prefab: ");
    result.message.AppendUtf8(importResult.catalogEntry.menuLabel);
    return result;
}

bool GltfImportService::PlaceCatalogEntry(ScenePlacementContext& ctx, const PrefabCatalogEntry& entry) const {
    const Utf8String path = ScenePathResolver::BuildRuntimePath("prefabs", entry.fileName.CStr());
    PrefabInstantiateOptions options{};
    options.assetsRoot = ScenePathResolver::AssetsRoot();
    options.position = {ctx.groundHit.x, 0.0F, ctx.groundHit.z};
    options.additive = true;
    options.pumpUntilReady = true;

    const PrefabInstantiateResult instantiateResult = ctx.sceneManager.InstantiatePrefab(path.CStr(), options);
    if (!instantiateResult.ready || instantiateResult.rootObjects.IsEmpty()) {
        return false;
    }

    ctx.instances.Register(instantiateResult.instanceId);
    for (std::size_t i = 0; i < instantiateResult.rootObjects.GetSize(); ++i) {
        GameObject* root = instantiateResult.rootObjects[i];
        if (root == nullptr) {
            continue;
        }
        ctx.content.TrackRoot(root);
        ctx.content.TrackPlaced(root, entry.captureMeshHint);
    }
    return true;
}

GltfImportService::ImportResult GltfImportService::ImportFromFilePicker(GameWorld& world) {
    Utf8String pickedPath{};
    if (!NativeFilePicker::TryPickGltfFile(pickedPath)) {
        ImportResult result{};
        result.message = Utf8String("Import cancelled.");
        return result;
    }
    const GltfPrefabImportResult importResult = importer.ImportFromGltf(world, pickedPath.CStr());
    return RegisterImportedPrefab(world, importResult);
}

GltfImportService::ImportResult GltfImportService::ImportAndPlace(ScenePlacementContext& ctx, const char* gltfPath) {
    ImportResult result{};
    const GltfPrefabImportResult importResult = importer.ImportFromGltf(ctx.world, gltfPath);
    result = RegisterImportedPrefab(ctx.world, importResult);
    if (!result.ok) {
        return result;
    }
    if (!PlaceCatalogEntry(ctx, result.catalogEntry)) {
        result.ok = false;
        result.message = Utf8String("Imported prefab but placement failed.");
        return result;
    }
    result.message = Utf8String("Imported and placed glTF prefab.");
    if (ctx.setStatus != nullptr) {
        ctx.setStatus(result.message.CStr(), ctx.statusUserData);
    }
    return result;
}

GltfImportService::ImportResult GltfImportService::ImportFromFilePickerAndPlace(
        GameWorld& /*world*/,
        ScenePlacementContext& ctx) {
    Utf8String pickedPath{};
    if (!NativeFilePicker::TryPickGltfFile(pickedPath)) {
        ImportResult result{};
        result.message = Utf8String("Import cancelled.");
        return result;
    }
    return ImportAndPlace(ctx, pickedPath.CStr());
}

}  // namespace Spark
