#pragma once

#include "spark/core/Utility.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/demo/platformer2d/Platformer2DBulletPool.hpp"
#include "spark/demo/platformer2d/Platformer2DExplosionFx.hpp"
#include "spark/demo/platformer2d/Platformer2DHealthHud.hpp"
#include "spark/ecs/components/camera/Camera2DComponent.hpp"
#include "spark/audio/SoundClip.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/audio/SoundFileLoader.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/ecs/components/gameplay/DamageableComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/physics/2d/HingeJoint2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/colliders/ColliderBakePipeline2D.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"
#include "spark/render/sprites2d/SpriteLighting2D.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>

namespace Spark {

/**
 * Procedural maze + spatial-hash physics, gem collectibles, and ghost enemies with basic chase AI.
 * Sprites use Kenney *Tiny Dungeon* when present under `assets/sprites/kenney_tiny-dungeon/`.
 */
class BroadPhase2DDemo {
public:
    static constexpr int kMazeLogicalW = 17;
    static constexpr int kMazeLogicalH = 13;
    static constexpr int kCorridorFloorCells = 3;
    static constexpr int kMazeStride = kCorridorFloorCells + 1;
    static constexpr int kMazeW = kMazeLogicalW * kMazeStride - 1;
    static constexpr int kMazeH = kMazeLogicalH * kMazeStride - 1;
    static constexpr float kCellWorld = 5.0F;
    static constexpr float kCameraHalfExtentInCells = 3.4F;

    static constexpr int kEnemyCount = 12;
    static constexpr float kPlayerMaxHealth = 100.0F;
    static constexpr float kEnemyBulletDamage = 8.0F;
    static constexpr float kPlayerHurtCooldownSeconds = 0.58F;
    static constexpr float kPlayerHalfW = 0.34F * kCellWorld;
    static constexpr float kPlayerHalfH = 0.34F * kCellWorld;
    static constexpr float kEnemyHalfW = 0.30F * kCellWorld;
    static constexpr float kEnemyHalfH = 0.30F * kCellWorld;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Unload(Spark::GameWorld& w);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);
    void Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context);

private:
    struct MazeEnemy {
        bool alive = false;
        Spark::GameObject* go = nullptr;
        Spark::TransformComponent* tr = nullptr;
        Spark::Rigidbody2DComponent* rb = nullptr;
        Spark::Vector2 homePos{};
        float shootCooldown = 0.0F;
        float wanderPhase = 0.0F;
        std::uint32_t spriteTile = 0U;
    };

    [[nodiscard]] Platformer2D::BulletProfile MakePlayerBulletProfile() const noexcept;
    [[nodiscard]] Platformer2D::BulletProfile MakeEnemyBulletProfile() const noexcept;
    void SpawnEnemies(Spark::GameWorld& world, const Spark::Array<Spark::Vector2>& spawnPoints);
    void TickEnemies(float deltaSeconds, float playerX, float playerY);
    void ResolveCombat(Spark::GameWorld& world, Spark::IEngineContext& context, float playerX, float playerY);

    Spark::Array<Spark::GameObject*> roots{};
    Spark::Array<Spark::GameObject*> gemObjects{};
    Spark::Array<Spark::Vector2> gemBasePositions{};
    Spark::Array<MazeEnemy> enemies{};
    Spark::Camera2D camera{};
    Spark::SharedPtr<Spark::Texture2D> dungeonAtlasTex{};
    Spark::SharedPtr<Spark::Texture2D> bulletTex{};
    Spark::SharedPtr<Spark::Texture2D> enemyBulletTex{};
    Spark::SharedPtr<Spark::Texture2D> hudWhiteTex{};
    Spark::Array<Spark::SharedPtr<Spark::Texture2D>> enemySpriteTextures{};
    Array<Collider2D> staticColliders{};
    SpatialHashGrid2D broadGrid{};
    Array<std::uint32_t> queryScratch{};
    int wallCount = 0;
    Spark::GameObject* playerGo = nullptr;
    Spark::TransformComponent* playerTr = nullptr;
    Spark::Rigidbody2DComponent* playerRb = nullptr;
    Spark::HealthComponent* playerHealth = nullptr;
    Spark::DamageableComponent* playerDamageable = nullptr;
    Spark::GameObject* mainCameraGo = nullptr;
    Spark::Camera2DComponent* cameraComp = nullptr;
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    Platformer2D::HealthHud healthHud{};
    float sceneTime = 0.0F;
    float playerHurtCooldown = 0.0F;
    float aimX = 1.0F;
    float aimY = 0.0F;
    float mazeOriginX = 0.0F;
    float mazeOriginY = 0.0F;
    int gemsCollected = 0;
    int gemsTotal = 0;
    int enemiesDefeated = 0;
    std::uint32_t lastBroadCandidates = 0;
    std::uint32_t lastNarrowHits = 0;
    PhysicsSubsystem physics{};
    Platformer2D::BulletPool playerBullets{};
    Platformer2D::BulletPool enemyBullets{};
    Platformer2D::ExplosionFx explosions{};
    Spark::SoundEngine* mazeAudioEngine = nullptr;
    Spark::SharedPtr<Spark::SoundClip> sfxCoin{};
    Spark::SharedPtr<Spark::SoundClip> sfxHurt{};
    Spark::SharedPtr<Spark::SoundClip> sfxExplosion{};
};

}  // namespace Spark
