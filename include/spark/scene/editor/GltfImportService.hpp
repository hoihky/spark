#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/editor/ScenePlacementActions.hpp"
#include "spark/scene/prefab/GltfPrefabImporter.hpp"
#include "spark/scene/prefab/PrefabCatalog.hpp"

namespace Spark {

/**
 * Editor facade: import glTF into prefabs, register catalog entries, and place instances.
 * Composes <c>GltfPrefabImporter</c> + <c>PrefabCatalog</c> + placement commands.
 */
class GltfImportService final {
public:
    struct ImportResult {
        bool ok = false;
        Utf8String message{};
        PrefabCatalogEntry catalogEntry{};
    };

    void Bind(PrefabCatalog* inCatalog, ScenePlacementActionRegistry* inPlacementActions) noexcept;

    /** File picker → prefab bake → catalog registration. */
    [[nodiscard]] ImportResult ImportFromFilePicker(GameWorld& world);

    /** Import + instantiate prefab at <c>ctx.groundHit</c>. */
    [[nodiscard]] ImportResult ImportAndPlace(ScenePlacementContext& ctx, const char* gltfPath);

    /** File picker + place at <c>ctx.groundHit</c>. */
    [[nodiscard]] ImportResult ImportFromFilePickerAndPlace(GameWorld& world, ScenePlacementContext& ctx);

private:
    [[nodiscard]] ImportResult RegisterImportedPrefab(
            GameWorld& world,
            const GltfPrefabImportResult& importResult) const;

    [[nodiscard]] bool PlaceCatalogEntry(ScenePlacementContext& ctx, const PrefabCatalogEntry& entry) const;

    GltfPrefabImporter importer{};
    PrefabCatalog* catalog = nullptr;
    ScenePlacementActionRegistry* placementActions = nullptr;
};

}  // namespace Spark
