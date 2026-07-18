#pragma once

#include "spark/core/Utility.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/camera/Camera2DRigComponent.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/ecs/components/animation/Sprite2DCharacterAnimFsmComponent.hpp"
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <format>

namespace Spark {

class Platformer2DDemo {
public:
    static constexpr int kPlatformCount = 16;
    static constexpr float kPlayerHalfW = 0.40F;
    static constexpr float kPlayerHalfH = 0.54F;
    /** Top of the wide floor slab (`kPlatforms[0][3]`). */
    static constexpr float kGroundSurfaceY = 0.0F;
    /** Kenney `platformPack_tileNNN.png` indices per platform (surfaces + blocks + fill). */
    static constexpr std::uint32_t kPlatformTileNumbers[kPlatformCount] = {
            1U,  1U,  2U,  3U,  20U, 40U, 2U,  3U,  1U,  4U,  20U, 40U, 3U,  20U, 1U,  2U,
    };
    /**
     * Axis-aligned solids (x0,y0)-(x1,y1), Y-up. Main route: floor → low steps → stair to summit (~7.5).
     * No tall colliders on the path (max single-step vertical ~1.35 so jump height ~2.5 is always enough).
     */
    static constexpr float kPlatforms[kPlatformCount][4] = {
            {-12.0F, -3.25F, 54.0F, 0.0F},
            {-7.25F, 0.2F, -0.2F, 0.95F},
            {0.05F, 0.9F, 4.35F, 1.52F},
            {5.65F, 1.95F, 9.15F, 2.42F},
            {10.35F, 2.85F, 14.85F, 3.38F},
            {16.1F, 3.82F, 20.9F, 4.32F},
            {22.35F, 4.68F, 27.85F, 5.22F},
            {30.2F, 5.82F, 36.25F, 6.38F},
            {39.35F, 6.92F, 47.25F, 7.48F},
            {17.85F, 0.52F, 24.15F, 1.08F},
            {26.4F, 0.82F, 32.1F, 1.38F},
            {7.85F, 0.18F, 11.15F, 0.62F},
            // Short scenery only (old tall wall blocked the whole start corridor — not jumpable).
            {-11.0F, 0.0F, -9.2F, 1.75F},
            {33.85F, 3.15F, 38.65F, 3.68F},
            {41.5F, 3.95F, 48.25F, 4.48F},
            {13.85F, 5.05F, 17.65F, 5.55F},
    };
    static constexpr int kGemCount = 16;
    /** World XY for collectible gems (green diamond `platformPack_item003`). */
    static constexpr float kGemSpawns[kGemCount][2] = {
            {-5.2F, 1.25F},
            {2.35F, 1.95F},
            {7.4F, 2.85F},
            {12.6F, 3.75F},
            {18.5F, 4.75F},
            {25.1F, 5.65F},
            {33.2F, 6.85F},
            {43.3F, 8.05F},
            {21.0F, 1.55F},
            {29.25F, 1.65F},
            {9.5F, 0.95F},
            {36.2F, 4.25F},
            {-6.5F, 2.15F},
            {35.25F, 4.05F},
            {15.75F, 6.05F},
            {45.0F, 5.35F},
    };
    /**
     * On the main floor slab only. The low step (x in [-7.25, -0.2], y in [0.2, 0.95]) overlaps spawns near
     * x = -5 together with the floor, so the solver mispredicts rest height. The scenery block ends at x = -9.2;
     * the player half-width is kPlayerHalfW — choose x so the AABB misses both (e.g. -8.5).
     */
    static constexpr float kPlayerSpawnX = -8.5F;
    static constexpr float kFallRespawnY = -8.0F;
    static constexpr float kGemDrawScale = 0.68F;
    static constexpr float kGemCollectRadius = 0.62F;
    /** Layer bit for gem hurtboxes (static trigger circles); weapon queries use <c>LayerBit(2)</c> vs this. */
    static constexpr std::uint16_t kGemHurtboxCategoryBits = CollisionFilter2D::LayerBit(1);
    static constexpr std::uint32_t kPlayerAttackClipIndex = 2U;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);


    void Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    Array<PhysicsQueryHit2D> attackArcHitsScratch{};
    Spark::Array<Spark::GameObject*> gemObjects{};
    Spark::SharedPtr<Spark::Texture2D> gemTex{};
    bool gemUsingKenneyPng = false;
    int gemsCollected = 0;
    int gemsTotal = 0;
    Spark::Array<Spark::GameObject*> roots{};
    Spark::GameObject* mainCameraGo = nullptr;
    Spark::Camera2DRigComponent* cameraRig = nullptr;
    Spark::SharedPtr<Spark::Texture2D> platformTilesTex{};
    bool platformUsingKenneyTilesheet = false;
    Spark::SharedPtr<Spark::Texture2D> playerAtlasTex{};
    Spark::GameObject* playerObject = nullptr;
    Spark::TransformComponent* playerTr = nullptr;
    Spark::Rigidbody2DComponent* playerRb = nullptr;
    SpriteAnimatorComponent* playerAnim = nullptr;
    Sprite2DCharacterAnimFsmComponent* playerCharFsm = nullptr;
    float playerBaseScaleX = kPlayerHalfW * 2.0F;
    float playerBaseScaleY = kPlayerHalfH * 2.0F;
    bool facingLeft = false;
    bool playerUsingKenneyAtlas = false;
    /** Uniform-grid columns for `player_atlas` (3 = idle+walk only; 5 adds happy+duck combat frames). */
    std::uint32_t playerAtlasColumns = 5U;
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    float fpsSmoothed = 0.0F;
    bool goalReached = false;
    float sceneTime = 0.0F;

};

}  // namespace Spark
