#include "spark/scene/tilemap/KenneyTinyDungeonGameplay.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"
#include "spark/scene/tilemap/TilemapLayer.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <cmath>
#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] float CellDistanceSqToWorld(
        const TilemapGridFrame& frame,
        const GridPathfinder::Cell& cell,
        const Vector2& worldXY) noexcept {
    const Vector2 center = frame.CellCenterToWorldXY(cell);
    const float dx = center.x - worldXY.x;
    const float dy = center.y - worldXY.y;
    return dx * dx + dy * dy;
}

}  // namespace

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

std::uint32_t FindKenneyDungeonLayerIndex(const TilemapComponent& tilemap) noexcept {
    for (std::uint32_t li = 0; li < tilemap.GetLayerCount(); ++li) {
        const char* name = tilemap.GetLayer(li).name.CStr();
        if (name != nullptr && std::strcmp(name, "Dungeon") == 0) {
            return li;
        }
    }
    return 0U;
}

bool IsKenneySandMapCell(
        const TilemapComponent& tilemap,
        const std::uint32_t dungeonLayerIndex,
        const std::int32_t x,
        const std::int32_t y) noexcept {
    if (x < 0 || y < 0) {
        return false;
    }
    const std::uint32_t ux = static_cast<std::uint32_t>(x);
    const std::uint32_t uy = static_cast<std::uint32_t>(y);
    if (ux >= tilemap.GetMapWidth() || uy >= tilemap.GetMapHeight()) {
        return false;
    }
    const TileCell cell = tilemap.GetTileCell(dungeonLayerIndex, ux, uy);
    if (!cell.HasVisual()) {
        return false;
    }
    return IsKenneyTinyDungeonWalkableFloorTile(cell.GetPaintTileId());
}

void CollectReachableKenneySandCells(
        const TilemapComponent& tilemap,
        const std::uint32_t dungeonLayerIndex,
        const GridPathfinder::Cell& start,
        Array<GridPathfinder::Cell>& out) noexcept {
    out.Clear();
    if (!IsKenneySandMapCell(tilemap, dungeonLayerIndex, start.x, start.y)) {
        return;
    }
    const std::int32_t width = static_cast<std::int32_t>(tilemap.GetMapWidth());
    const std::int32_t height = static_cast<std::int32_t>(tilemap.GetMapHeight());
    if (width <= 0 || height <= 0) {
        return;
    }

    Array<std::uint8_t> visited{};
    visited.Resize(static_cast<std::size_t>(width * height));
    for (std::size_t i = 0; i < visited.GetSize(); ++i) {
        visited[i] = 0U;
    }
    auto visitIndex = [&](const std::int32_t x, const std::int32_t y) noexcept -> std::size_t {
        return static_cast<std::size_t>(y * width + x);
    };

    Array<GridPathfinder::Cell> queue{};
    queue.PushBack(start);
    visited[visitIndex(start.x, start.y)] = 1U;
    out.PushBack(start);

    std::size_t head = 0U;
    while (head < queue.GetSize()) {
        const GridPathfinder::Cell current = queue[head++];
        const std::int32_t neighbors[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (const auto& offset : neighbors) {
            const GridPathfinder::Cell next{current.x + offset[0], current.y + offset[1]};
            if (!IsKenneySandMapCell(tilemap, dungeonLayerIndex, next.x, next.y)) {
                continue;
            }
            const std::size_t index = visitIndex(next.x, next.y);
            if (visited[index] != 0U) {
                continue;
            }
            visited[index] = 1U;
            queue.PushBack(next);
            out.PushBack(next);
        }
    }
}

bool PickKenneySandSpawnCell(
        const TilemapComponent& tilemap,
        const std::uint32_t dungeonLayerIndex,
        const TilemapGridFrame& frame,
        const Vector2& hintWorldXY,
        const std::size_t minReachableCells,
        GridPathfinder::Cell& outCell) noexcept {
    const std::int32_t width = static_cast<std::int32_t>(tilemap.GetMapWidth());
    const std::int32_t height = static_cast<std::int32_t>(tilemap.GetMapHeight());
    if (width <= 0 || height <= 0) {
        return false;
    }

    const std::size_t cellCount = static_cast<std::size_t>(width * height);
    Array<std::uint8_t> visited{};
    visited.Resize(cellCount);
    for (std::size_t i = 0; i < cellCount; ++i) {
        visited[i] = 0U;
    }
    auto visitIndex = [&](const std::int32_t x, const std::int32_t y) noexcept -> std::size_t {
        return static_cast<std::size_t>(y * width + x);
    };

    GridPathfinder::Cell bestRegionSeed{};
    std::size_t bestRegionSize = 0U;

    for (std::int32_t y = 0; y < height; ++y) {
        for (std::int32_t x = 0; x < width; ++x) {
            const std::size_t startIndex = visitIndex(x, y);
            if (visited[startIndex] != 0U || !IsKenneySandMapCell(tilemap, dungeonLayerIndex, x, y)) {
                continue;
            }
            Array<GridPathfinder::Cell> region{};
            CollectReachableKenneySandCells(tilemap, dungeonLayerIndex, {x, y}, region);
            for (std::size_t ri = 0; ri < region.GetSize(); ++ri) {
                const GridPathfinder::Cell& c = region[ri];
                visited[visitIndex(c.x, c.y)] = 1U;
            }
            if (region.GetSize() > bestRegionSize) {
                bestRegionSize = region.GetSize();
                bestRegionSeed = {x, y};
            }
        }
    }

    if (bestRegionSize < minReachableCells) {
        return false;
    }

    Array<GridPathfinder::Cell> primary{};
    CollectReachableKenneySandCells(tilemap, dungeonLayerIndex, bestRegionSeed, primary);
    if (primary.GetSize() < minReachableCells) {
        return false;
    }

    float bestDistSq = 0.0F;
    bool found = false;
    for (std::size_t i = 0; i < primary.GetSize(); ++i) {
        const float distSq = CellDistanceSqToWorld(frame, primary[i], hintWorldXY);
        if (!found || distSq < bestDistSq) {
            bestDistSq = distSq;
            outCell = primary[i];
            found = true;
        }
    }
    return found;
}

}  // namespace Spark
