#include "spark/scene/tilemap/KenneyTinyDungeonGameplay.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/tilemap/TilemapLayer.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <cstring>

namespace Spark {

bool IsKenneyTinyDungeonSandFloorTile(const std::uint16_t atlasTileId) noexcept {
    switch (atlasTileId) {
        case 120U:
        case 121U:
        case 122U:
        case 123U:
            return true;
        default:
            return false;
    }
}

bool IsKenneyTinyDungeonWalkableFloorTile(const std::uint16_t atlasTileId) noexcept {
    if (IsKenneyTinyDungeonSandFloorTile(atlasTileId)) {
        return true;
    }
    if (atlasTileId == 126U) {
        return true;
    }
    /** Town interior plates on <c>sampleMap.tmx</c> (TMX GIDs 49–54, 60). Not brick walls (88+). */
    switch (atlasTileId) {
        case 72U:
        case 73U:
        case 74U:
        case 75U:
        case 76U:
        case 77U:
        case 83U:
            return true;
        default:
            return false;
    }
}

void ConfigureKenneyTinyDungeonGameplayTileset(Tileset& tileset) noexcept {
    tileset.EnsureDefinitions();
    const std::uint32_t cellCount = tileset.GetCellCount();
    for (std::uint32_t i = 0; i < cellCount; ++i) {
        TileDefinition& def = tileset.Definition(static_cast<std::uint16_t>(i));
        def.flags = TileDefinitionFlags::None;
        if (IsKenneyTinyDungeonWalkableFloorTile(static_cast<std::uint16_t>(i))) {
            def.collisionShape = TileCollisionShape::None;
            continue;
        }
        def.flags = TileDefinitionFlags::BlocksPathfinding;
        def.collisionShape = TileCollisionShape::FullCell;
    }
}

void ApplyKenneyTinyDungeonGameplayLayerFlags(TilemapComponent& tilemap) noexcept {
    for (std::uint32_t li = 0; li < tilemap.GetLayerCount(); ++li) {
        TilemapLayer& layer = tilemap.GetLayer(li);
        const char* name = layer.name.CStr();
        if (name == nullptr) {
            continue;
        }
        if (std::strcmp(name, "Carts") == 0) {
            layer.contributeCollision = false;
            layer.contributeGameplayGrid = false;
        } else if (std::strcmp(name, "Objects") == 0) {
            layer.contributeGameplayGrid = false;
        }
    }
}

void ApplyKenneyTinyDungeonGameplayToTilemap(GameObject& levelRoot) noexcept {
    TilemapComponent* tilemap = levelRoot.GetComponent<TilemapComponent>();
    if (tilemap == nullptr) {
        return;
    }
    if (SharedPtr<Tileset> tileset = tilemap->GetTileset(); tileset) {
        ConfigureKenneyTinyDungeonGameplayTileset(*tileset);
    }
    ApplyKenneyTinyDungeonGameplayLayerFlags(*tilemap);
}

}  // namespace Spark
