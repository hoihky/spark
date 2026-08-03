#pragma once

#include "spark/core/Utility.hpp"
#include "spark/demo/DemoFoundation.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/demo/platformer2d/Platformer2DBulletPool.hpp"
#include "spark/demo/platformer2d/Platformer2DConfig.hpp"
#include "spark/demo/platformer2d/Platformer2DEnemySquad.hpp"
#include "spark/demo/platformer2d/Platformer2DExplosionFx.hpp"
#include "spark/demo/platformer2d/Platformer2DHealthHud.hpp"
#include "spark/demo/platformer2d/Platformer2DPlayerCombat.hpp"
#include "spark/ecs/components/camera/Camera2DRigComponent.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/ecs/components/animation/Sprite2DCharacterAnimFsmComponent.hpp"
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"
#include "spark/audio/SoundClip.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"

#include <cmath>

namespace Spark {

/**
 * Teaching-oriented 2D platformer shell. Gameplay is split into small subsystems under
 * <c>spark/demo/platformer2d/</c> (object pools, squad AI, HUD observer, player combat facade).
 *
 * Patterns demonstrated:
 * - **Facade** — this class wires subsystems without embedding every detail.
 * - **Object pool** — <c>Platformer2D::BulletPool</c>, <c>Platformer2D::ExplosionFx</c>.
 * - **Strategy** — <c>Platformer2D::BulletProfile</c> for player vs enemy shots.
 * - **Observer / presentation** — <c>Platformer2D::HealthHud</c> mirrors <c>HealthComponent</c>.
 */
class Platformer2DDemo {
public:
    static constexpr int kPlatformCount = 16;
    static constexpr float kPlayerHalfW = Platformer2D::Config::kPlayerHalfW;
    static constexpr float kPlayerHalfH = Platformer2D::Config::kPlayerHalfH;
    static constexpr float kGroundSurfaceY = 0.0F;
    static constexpr std::uint32_t kPlatformTileNumbers[kPlatformCount] = {
            1U,  1U,  2U,  3U,  20U, 40U, 2U,  3U,  1U,  4U,  20U, 40U, 3U,  20U, 1U,  2U,
    };
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
            {-11.0F, 0.0F, -9.2F, 1.75F},
            {33.85F, 3.15F, 38.65F, 3.68F},
            {41.5F, 3.95F, 48.25F, 4.48F},
            {13.85F, 5.05F, 17.65F, 5.55F},
    };
    static constexpr int kGemCount = 16;
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
    static constexpr float kPlayerSpawnX = -8.5F;
    static constexpr float kFallRespawnY = -8.0F;
    static constexpr float kGemDrawScale = 0.68F;
    static constexpr float kGemCollectRadius = 0.62F;
    static constexpr std::uint16_t kGemHurtboxCategoryBits = Platformer2D::Config::kGemHurtboxCategoryBits;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Unload(Spark::GameWorld& w);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);
    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);

private:
    [[nodiscard]] Platformer2D::BulletProfile MakePlayerBulletProfile() const noexcept;
    [[nodiscard]] Platformer2D::BulletProfile MakeEnemyBulletProfile() const noexcept;

    DemoRootCollection roots{};
    Spark::Array<Spark::GameObject*> gemObjects{};
    Spark::SharedPtr<Spark::Texture2D> gemTex{};
    Spark::SharedPtr<Spark::Texture2D> platformTilesTex{};
    Spark::SharedPtr<Spark::Texture2D> playerAtlasTex{};
    Spark::SharedPtr<Spark::Texture2D> enemyAtlasTex{};
    Spark::SharedPtr<Spark::Texture2D> playerBulletTex{};
    Spark::SharedPtr<Spark::Texture2D> enemyBulletTex{};
    Spark::SharedPtr<Spark::Texture2D> hudWhiteTex{};
    bool platformUsingKenneyTilesheet = false;
    std::uint32_t playerAtlasColumns = 5U;
    std::uint32_t enemyAtlasColumns = 1U;
    Spark::Vector4 enemyIdleUv{0.0F, 0.0F, 1.0F, 1.0F};
    Spark::Vector4 enemyAttackUv{0.0F, 0.0F, 1.0F, 1.0F};

    Spark::GameObject* mainCameraGo = nullptr;
    Spark::Camera2DRigComponent* cameraRig = nullptr;
    Spark::GameObject* playerObject = nullptr;
    Spark::TransformComponent* playerTr = nullptr;
    Spark::Rigidbody2DComponent* playerRb = nullptr;
    Spark::HealthComponent* playerHealth = nullptr;
    Spark::DamageableComponent* playerDamageable = nullptr;
    SpriteAnimatorComponent* playerAnim = nullptr;
    Sprite2DCharacterAnimFsmComponent* playerCharFsm = nullptr;
    float playerBaseScaleX = kPlayerHalfW * 2.0F;
    float playerBaseScaleY = kPlayerHalfH * 2.0F;
    bool facingLeft = false;

    Spark::SharedPtr<Spark::SoundClip> sfxJump{};
    Spark::SharedPtr<Spark::SoundClip> sfxCoin{};
    Spark::SharedPtr<Spark::SoundClip> sfxExplosion{};
    Spark::SharedPtr<Spark::SoundClip> sfxHurt{};

    Spark::SoundEngine* audioEngine = nullptr;

    Platformer2D::BulletPool playerBullets{};
    Platformer2D::BulletPool enemyBullets{};
    Platformer2D::ExplosionFx explosions{};
    Platformer2D::HealthHud healthHud{};
    Platformer2D::EnemySquad enemySquad{};
    Platformer2D::PlayerCombat playerCombat{};

    PhysicsSubsystem physics{};

    int gemsCollected = 0;
    int gemsTotal = 0;
    bool goalReached = false;
    float sceneTime = 0.0F;
};

}  // namespace Spark
