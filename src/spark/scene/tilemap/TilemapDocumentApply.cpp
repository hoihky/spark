#include "spark/scene/tilemap/TilemapDocumentApply.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/tilemap/TilemapFileResolve.hpp"
#include "spark/scene/tilemap/TilemapLayerSortMode.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] Utf8String ResolveTilesetTexturePath(const TilemapDocumentTileset& primary) {
    Utf8String resolved = ResolveTilemapAssetPath(primary.imagePath.CStr());
    if (resolved.IsEmpty()) {
        return {};
    }
    const char* path = resolved.CStr();
    if (path == nullptr || std::strstr(path, "kenney_tiny-dungeon") == nullptr) {
        return resolved;
    }
    static const char* kPackedRelative = "sprites/kenney_tiny-dungeon/Tilemap/tilemap_packed.png";
    const Utf8String packedResolved = ResolveTilemapAssetPath(kPackedRelative);
    return packedResolved.IsEmpty() ? resolved : packedResolved;
}

[[nodiscard]] bool UsesKenneyPackedGrid(const char* texturePath) noexcept {
    return texturePath != nullptr && std::strstr(texturePath, "tilemap_packed.png") != nullptr;
}

/**
 * Kenney Tiny Dungeon packed atlas indices used as walkable floor in sampleMap-style dungeon
 * layers. Wall autotiles (e.g. 108–117), hazards, and generic caps (120, 123) are excluded even
 * when they share a row with floor variants in the sheet.
 */
[[nodiscard]] bool IsKenneyTinyDungeonWalkableFloorTile(const std::uint16_t tileId) noexcept {
    switch (tileId) {
        case 72U:
        case 73U:
        case 74U:
        case 75U:
        case 76U:
        case 77U:
        case 81U:
        case 83U:
        case 88U:
        case 121U:
        case 122U:
        case 124U:
        case 125U:
        case 126U:
            return true;
        default:
            return false;
    }
}

void ConfigureKenneyTinyDungeonGameplayTileset(Tileset& tileset) {
    tileset.EnsureDefinitions();
    const std::uint32_t cellCount = tileset.GetCellCount();
    for (std::uint32_t i = 0; i < cellCount; ++i) {
        TileDefinition& def = tileset.Definition(static_cast<std::uint16_t>(i));
        def.collisionShape = TileCollisionShape::None;
        def.flags = TileDefinitionFlags::None;
        if (!IsKenneyTinyDungeonWalkableFloorTile(static_cast<std::uint16_t>(i))) {
            def.flags = TileDefinitionFlags::BlocksPathfinding;
        }
    }
}

void ApplyKenneyTinyDungeonGameplayLayerFlags(TilemapComponent& tilemap) {
    for (std::uint32_t li = 0; li < tilemap.GetLayerCount(); ++li) {
        TilemapLayer& layer = tilemap.GetLayer(li);
        const char* name = layer.name.CStr();
        if (name != nullptr &&
            (std::strcmp(name, "Objects") == 0 || std::strcmp(name, "Carts") == 0)) {
            layer.contributeGameplayGrid = false;
        }
    }
}

[[nodiscard]] const TilemapDocumentTileset* PrimaryTileset(const TilemapDocument& document) noexcept {
    if (document.tilesets.IsEmpty()) {
        return nullptr;
    }
    const TilemapDocumentTileset* best = nullptr;
    for (std::size_t i = 0; i < document.tilesets.GetSize(); ++i) {
        const TilemapDocumentTileset& candidate = document.tilesets[i];
        if (candidate.imagePath.IsEmpty()) {
            continue;
        }
        if (best == nullptr || candidate.firstGid < best->firstGid) {
            best = &candidate;
        }
    }
    return best != nullptr ? best : &document.tilesets[0];
}

[[nodiscard]] std::uint32_t ComputeTilesV(const TilemapDocumentTileset& set) noexcept {
    if (set.columns == 0U) {
        return 1U;
    }
    const std::uint32_t count = set.tileCount > 0U ? set.tileCount : set.columns;
    return (count + set.columns - 1U) / set.columns;
}

void SyncTileLayers(TilemapComponent& tilemap, const TilemapDocument& document) {
    while (tilemap.GetLayerCount() > document.tileLayers.GetSize() && tilemap.GetLayerCount() > 1U) {
        tilemap.RemoveLayer(tilemap.GetLayerCount() - 1U);
    }
    while (tilemap.GetLayerCount() < document.tileLayers.GetSize()) {
        static_cast<void>(tilemap.AddLayer("Layer"));
    }
    for (std::size_t li = 0; li < document.tileLayers.GetSize(); ++li) {
        const TilemapDocumentTileLayer& src = document.tileLayers[li];
        TilemapLayer& dst = tilemap.GetLayer(static_cast<std::uint32_t>(li));
        dst.name = src.name;
        dst.visible = src.visible;
        dst.orderInLayerOffset = src.orderInLayerOffset;
        dst.contributeCollision = src.contributeCollision;
        dst.contributeGameplayGrid = src.contributeGameplayGrid;
        dst.sortMode = TilemapLayerSortMode::GridOrder;
        dst.cells = src.cells;
    }
}

}  // namespace

TilemapDocumentApplyResult ApplyTilemapDocument(
        const TilemapDocument& document,
        GameObject& owner,
        GameWorld& world,
        const TilemapDocumentApplyOptions& options) {
    TilemapDocumentApplyResult result{};
    if (document.mapWidth == 0U || document.mapHeight == 0U || document.tileLayers.IsEmpty()) {
        result.errorMessage = Utf8String("Tilemap document is empty");
        return result;
    }
    const TilemapDocumentTileset* primary = PrimaryTileset(document);
    if (primary == nullptr || primary->imagePath.IsEmpty()) {
        result.errorMessage = Utf8String("Tilemap document has no tileset image");
        return result;
    }

    const Utf8String texturePath = ResolveTilesetTexturePath(*primary);
    if (texturePath.IsEmpty()) {
        result.errorMessage = Utf8String("Failed to resolve tileset image path");
        result.errorMessage.AppendUtf8(primary->imagePath.CStr());
        return result;
    }

    SharedPtr<Texture2D> atlas = world.LoadTexture(texturePath.CStr());
    if (!atlas) {
        result.errorMessage = Utf8String("Failed to load tileset texture");
        result.errorMessage.AppendUtf8(texturePath.CStr());
        return result;
    }
    static_cast<void>(world.RegisterTexture(atlas, texturePath.CStr()));

    const std::uint32_t tilesU = primary->columns > 0U ? primary->columns : 1U;
    std::uint32_t tilesV = ComputeTilesV(*primary);
    if (primary->tileCount > 0U && tilesU > 0U) {
        tilesV = (primary->tileCount + tilesU - 1U) / tilesU;
    }
    const float pixelsPerWorldUnit =
            options.pixelsPerWorldUnit > 1.0e-6F ? options.pixelsPerWorldUnit : 16.0F;
    const float derivedTileWorld = static_cast<float>(primary->tileWidth) / pixelsPerWorldUnit;
    const float tileWorld =
            document.tileWorldSize > 1.0e-6F ? document.tileWorldSize : derivedTileWorld;

    TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (tilemap == nullptr) {
        if (!options.createComponentsIfMissing) {
            result.errorMessage = Utf8String("Owner has no TilemapComponent");
            return result;
        }
        tilemap = owner.AddComponent<TilemapComponent>(
                atlas, document.mapWidth, document.mapHeight, tilesU, tilesV, tileWorld, document.sortOrderBase);
    } else {
        tilemap->Resize(document.mapWidth, document.mapHeight);
        tilemap->SetSortOrderBase(document.sortOrderBase);
        tilemap->SetTileWorldSize(tileWorld);
    }

    std::uint32_t imageW = primary->imageWidth;
    std::uint32_t imageH = primary->imageHeight;
    std::uint32_t margin = primary->margin;
    std::uint32_t spacing = primary->spacing;
    if (UsesKenneyPackedGrid(texturePath.CStr())) {
        margin = 0U;
        spacing = 0U;
        imageW = atlas->GetWidth();
        imageH = atlas->GetHeight();
    } else if (imageW == 0U || imageH == 0U) {
        imageW = atlas->GetWidth();
        imageH = atlas->GetHeight();
    }

    SharedPtr<Tileset> tileset = CreateTilesetFromAtlas(atlas, tilesU, tilesV);
    tileset->SetAtlasPadding(static_cast<float>(margin), static_cast<float>(spacing));
    tileset->SetTiledAtlasLayout(
            primary->tileWidth,
            primary->tileHeight,
            imageW,
            imageH,
            primary->tileCount);
    tilemap->SetTileset(tileset);
    SyncTileLayers(*tilemap, document);
    if (UsesKenneyPackedGrid(texturePath.CStr())) {
        ConfigureKenneyTinyDungeonGameplayTileset(*tileset);
        ApplyKenneyTinyDungeonGameplayLayerFlags(*tilemap);
    }

    if (options.applyObjectLayers) {
        TilemapObjectLayerComponent* objects = owner.GetComponent<TilemapObjectLayerComponent>();
        if (objects == nullptr && options.createComponentsIfMissing) {
            objects = owner.AddComponent<TilemapObjectLayerComponent>();
        }
        if (objects != nullptr) {
            while (objects->GetLayerCount() > 0U) {
                objects->RemoveObjectLayer(0U);
            }
            for (std::size_t i = 0; i < document.objectLayers.GetSize(); ++i) {
                const std::uint32_t layerIndex = objects->AddObjectLayer(document.objectLayers[i].name.CStr());
                TilemapObjectLayer& dst = objects->GetLayer(layerIndex);
                dst.visible = document.objectLayers[i].visible;
                dst.markers = document.objectLayers[i].markers;
            }
        }
    }

    result.success = true;
    return result;
}

}  // namespace Spark
