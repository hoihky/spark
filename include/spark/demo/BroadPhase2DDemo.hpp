#pragma once

#include "spark/core/Utility.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/audio/SoundClip.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/audio/SoundFileLoader.hpp"
#include "spark/ecs/components/SpriteAnimatorComponent.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"
#include "spark/render/sprites2d/SpriteLighting2D.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Spark {

/**
 * Procedural maze + spatial-hash physics, plus pulsing gem collectibles using SpriteLighting2D (GPU sprite.frag).
 * Sprites use Kenney *Tiny Dungeon* `Tilemap/tilemap_packed.png` when present under `assets/sprites/kenney_tiny-dungeon/`.
 * Uses <c>SceneSpriteSortMode::SortOrderThenWorldY</c>: player and gems share <c>sortOrder</c> so occlusion follows world Y.
 */
class BroadPhase2DDemo {
public:
    /** Recursive maze size (odd). Each floor cell becomes `kCorridorFloorCells` wide in the physical grid. */
    static constexpr int kMazeLogicalW = 11;
    static constexpr int kMazeLogicalH = 9;
    /** Walkable width per logical corridor tile; walls between tiles stay **one** physical cell. */
    static constexpr int kCorridorFloorCells = 3;
    static constexpr int kMazeStride = kCorridorFloorCells + 1;
    /** Physical grid used for rendering / physics (`BuildPhysicalMazeWideFloorsThinWalls`). */
    static constexpr int kMazeW = kMazeLogicalW * kMazeStride - 1;
    static constexpr int kMazeH = kMazeLogicalH * kMazeStride - 1;
    /** World units per physical maze cell (sprites and colliders scale with this). */
    static constexpr float kCellWorld = 5.0F;
    /** Ortho half-height ≈ this many cells (camera is not tied to full maze size so sprites stay visible). */
    static constexpr float kCameraHalfExtentInCells = 3.4F;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);


    void Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    Spark::Array<Spark::GameObject*> roots{};
    Spark::Array<Spark::GameObject*> gemObjects{};
    Spark::Camera2D camera{};
    Spark::SharedPtr<Spark::Texture2D> dungeonAtlasTex{};
    Array<StaticCollider2D> staticColliders{};
    SpatialHashGrid2D broadGrid{};
    Array<std::uint32_t> queryScratch{};
    int wallCount = 0;
    Spark::GameObject* playerGo = nullptr;
    Spark::TransformComponent* playerTr = nullptr;
    Spark::Rigidbody2DComponent* playerRb = nullptr;
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    float fpsSmoothed = 0.0F;
    float sceneTime = 0.0F;
    int gemsCollected = 0;
    int gemsTotal = 0;
    std::uint32_t lastBroadCandidates = 0;
    std::uint32_t lastNarrowHits = 0;
    /** Cleared in <c>Unload</c> so background music stops when leaving the demo. */
    Spark::SoundEngine* mazeAudioEngine = nullptr;

};

}  // namespace Spark
