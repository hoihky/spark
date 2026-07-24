#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapAutotileComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapTileAnimatorComponent.hpp"
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"

#include <cstdint>

namespace Spark {

/**
 * Small overworld-style map demonstrating tile animation, stacked tilemap layers,
 * gameplay-grid pathfinding, and autotiled grass terrain.
 *
 * Left-click: set path goal (A* on baked walkability). Player follows the polyline.
 */
class TilemapShowcase2DDemo {
public:
    static constexpr int kCols = 22;
    static constexpr int kRows = 14;
    static constexpr float kTileWorld = 1.0F;

    static constexpr std::uint16_t kTileGrassPaint = 0U;
    static constexpr std::uint16_t kTileGrassEdge = 1U;
    static constexpr std::uint16_t kTileWall = 2U;
    static constexpr std::uint16_t kTileWaterA = 3U;
    static constexpr std::uint16_t kTileWaterB = 4U;
    static constexpr std::uint16_t kTileProp = 5U;
    static constexpr std::uint16_t kTilePlayerIcon = 6U;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Unload(Spark::GameWorld& w);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);
    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);

private:
    /** After TMX import, restores 22×14 procedural map size, showcase atlas, and layer setup. */
    void RestoreProceduralTilemap();
    void BuildLevel();
    void BuildObjectMarkers(Spark::TilemapObjectLayerComponent& objects, std::uint32_t layerIndex);
    void ClearPathMarkers();
    void ShowPath(const Array<GridPathfinder::Cell>& cells);
    [[nodiscard]] bool PickCell(Spark::IEngineContext& context, float mx, float my, int& outX, int& outY) const;

    Spark::Array<Spark::GameObject*> roots{};
    Spark::SharedPtr<Spark::Texture2D> atlasTex{};
    Spark::GameObject* boardGo = nullptr;
    Spark::TilemapComponent* tilemap = nullptr;
    Spark::TilemapGameplayGridComponent* gameplayGrid = nullptr;
    Spark::GameObject* playerGo = nullptr;
    Spark::GameObject* hudGo = nullptr;
    Spark::TextOverlayComponent* hudText = nullptr;
    Spark::Array<Spark::GameObject*> pathMarkers{};

    Spark::Camera2D camera{};
    Spark::Vector2 playerPos{3.5F, 3.5F};
    Spark::Vector2 pathTarget{3.5F, 3.5F};
    Array<GridPathfinder::Cell> pathCells{};
    std::size_t pathStep = 0;
    float moveSpeed = 4.5F;
    Utf8String tmxStatus{};
};

}  // namespace Spark
