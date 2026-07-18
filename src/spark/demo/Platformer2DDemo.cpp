#include "spark/demo/Platformer2DDemo.hpp"
#include "spark/demo/Platformer2DDemo_detail.hpp"
#include "spark/ecs/components/gameplay/DamageableComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/physics/2d/PhysicsMaterial2DComponent.hpp"
#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/components/camera/Camera2DRigComponent.hpp"
#include "spark/scene/SceneSubmit.hpp"
#include "spark/ecs/components/animation/Sprite2DCharacterAnimFsmComponent.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

namespace Spark {

namespace {

constexpr float kAttackArcRadius = 1.5F;
constexpr float kAttackArcHalfAngleRad = 0.96F;
constexpr float kEnemyHalfW = 0.36F;
constexpr float kEnemyHalfH = 0.42F;
constexpr float kEnemyDrawScale = 0.88F;
constexpr float kEnemyPatrolSpeed = 2.2F;
constexpr float kEnemyBobAmplitude = 0.05F;
constexpr float kEnemyShootRangeX = 14.0F;
constexpr float kEnemyShootRangeY = 7.5F;
constexpr float kBulletSpeed = 7.0F;
constexpr float kBulletHalfW = 0.055F;
constexpr float kBulletHalfH = 0.035F;
constexpr float kBulletLifetimeSeconds = 3.6F;
constexpr float kBulletDrawScale = 0.42F;
constexpr float kPlayerHurtCooldownSeconds = 1.15F;

}  // namespace

void Platformer2DDemo::DeactivateBullet(BulletSlot& b) noexcept
{
    b.active = false;
    b.vx = 0.0F;
    b.vy = 0.0F;
    b.age = 0.0F;
    b.lifetime = 0.0F;
    if (b.tr != nullptr) {
        b.tr->SetTranslation({-120.0F, -120.0F, 0.0F});
        b.tr->SetRotation(Spark::Quaternion::Identity);
    }
    if (b.spr != nullptr) {
        const Spark::Vector4 tint = b.spr->GetTint();
        b.spr->SetTint({tint.x, tint.y, tint.z, 0.0F});
    }
}

bool Platformer2DDemo::BoxOverlap(
        float ax,
        float ay,
        float ahx,
        float ahy,
        float bx,
        float by,
        float bhx,
        float bhy) noexcept
{
    return std::fabs(ax - bx) <= ahx + bhx && std::fabs(ay - by) <= ahy + bhy;
}

[[nodiscard]] bool Platformer2DDemo::TrySpawnEnemyBullet(
        EnemySlot& enemy,
        float playerX,
        float playerY) noexcept
{
    if (!enemy.alive || enemy.tr == nullptr) {
        return false;
    }
    const Spark::Vector3 epos = enemy.tr->GetLocalTransform().translation;
    float dx = playerX - epos.x;
    float dy = playerY - epos.y;
    const float len2 = dx * dx + dy * dy;
    if (len2 < 1.0e-4F) {
        dx = enemy.patrolDir;
        dy = 0.0F;
    } else {
        const float inv = 1.0F / std::sqrt(len2);
        dx *= inv;
        dy *= inv;
    }
    for (std::size_t bi = 0; bi < enemyBullets.GetSize(); ++bi) {
        BulletSlot& bl = enemyBullets[bi];
        if (bl.active) {
            continue;
        }
        bl.active = true;
        bl.vx = dx * kBulletSpeed;
        bl.vy = dy * kBulletSpeed;
        bl.age = 0.0F;
        bl.lifetime = kBulletLifetimeSeconds;
        bl.cx = epos.x + dx * (kEnemyHalfW * 0.95F);
        bl.cy = epos.y + dy * (kEnemyHalfW * 0.35F) + kEnemyHalfH * 0.08F;
        if (bl.tr != nullptr) {
            bl.tr->SetTranslation({bl.cx, bl.cy, 0.06F});
            bl.tr->SetUniformScale(kBulletDrawScale);
            const float angleZ = std::atan2(bl.vy, bl.vx);
            bl.tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitZ, angleZ));
        }
        if (bl.spr != nullptr) {
            bl.spr->SetTint({1.0F, 0.72F, 0.38F, 0.92F});
        }
        enemy.attackFlashTimer = 0.24F;
        return true;
    }
    return false;
}

void Platformer2DDemo::UpdateEnemies(float dt, float playerX, float playerY) noexcept
{
    for (std::size_t ei = 0; ei < enemies.GetSize(); ++ei) {
        EnemySlot& enemy = enemies[ei];
        if (!enemy.alive || enemy.tr == nullptr) {
            continue;
        }
        Spark::Vector3 pos = enemy.tr->GetLocalTransform().translation;
        pos.x += enemy.patrolDir * kEnemyPatrolSpeed * dt;
        if (pos.x <= enemy.patrolMinX) {
            pos.x = enemy.patrolMinX;
            enemy.patrolDir = 1.0F;
        } else if (pos.x >= enemy.patrolMaxX) {
            pos.x = enemy.patrolMaxX;
            enemy.patrolDir = -1.0F;
        }
        const float bob = std::sin(sceneTime * 3.2F + enemy.bobPhase) * kEnemyBobAmplitude;
        pos.y = enemy.baseY + bob;
        enemy.tr->SetTranslation(pos);
        const float sx = (enemy.patrolDir < 0.0F ? -kEnemyDrawScale : kEnemyDrawScale);
        enemy.tr->SetScale({sx, kEnemyDrawScale, 1.0F});

        if (enemy.spr != nullptr) {
            if (enemy.attackFlashTimer > 0.0F) {
                enemy.attackFlashTimer = std::max(0.0F, enemy.attackFlashTimer - dt);
                enemy.spr->SetUvRect(enemyAttackUv);
            } else {
                enemy.spr->SetUvRect(enemyIdleUv);
            }
        }

        enemy.fireCooldown -= dt;
        if (enemy.fireCooldown > 0.0F) {
            continue;
        }
        const float dx = playerX - pos.x;
        const float dy = playerY - pos.y;
        if (std::fabs(dx) > kEnemyShootRangeX || std::fabs(dy) > kEnemyShootRangeY) {
            continue;
        }
        const bool playerAhead =
                (enemy.patrolDir > 0.0F && dx > 0.75F) || (enemy.patrolDir < 0.0F && dx < -0.75F);
        if (!playerAhead) {
            continue;
        }
        if (TrySpawnEnemyBullet(enemy, playerX, playerY)) {
            enemy.fireCooldown = 2.35F + 0.45F * static_cast<float>(ei % 3U);
        } else {
            enemy.fireCooldown = 0.45F;
        }
    }
}

void Platformer2DDemo::UpdateEnemyBullets(float dt) noexcept
{
    constexpr float kCullX = 18.0F;
    constexpr float kCullY = 14.0F;
    for (std::size_t bi = 0; bi < enemyBullets.GetSize(); ++bi) {
        BulletSlot& bl = enemyBullets[bi];
        if (!bl.active) {
            continue;
        }
        bl.age += dt;
        if (bl.age >= bl.lifetime) {
            DeactivateBullet(bl);
            continue;
        }
        bl.cx += bl.vx * dt;
        bl.cy += bl.vy * dt;
        if (bl.tr != nullptr) {
            bl.tr->SetTranslation({bl.cx, bl.cy, 0.06F});
        }
        if (bl.spr != nullptr) {
            const float pulse = 0.78F + 0.22F * std::sin(bl.age * 18.0F);
            const float fade = 1.0F - std::clamp((bl.age - bl.lifetime * 0.72F) / (bl.lifetime * 0.28F), 0.0F, 1.0F);
            bl.spr->SetTint({1.0F, 0.68F + 0.12F * pulse, 0.32F, 0.55F + 0.35F * pulse * fade});
        }
        if (bl.cx < -kCullX || bl.cx > 54.0F || bl.cy < -kCullY || bl.cy > 12.0F) {
            DeactivateBullet(bl);
        }
    }
}

void Platformer2DDemo::ResolveEnemyBulletHits(float playerX, float playerY) noexcept
{
    for (std::size_t bi = 0; bi < enemyBullets.GetSize(); ++bi) {
        BulletSlot& bl = enemyBullets[bi];
        if (!bl.active) {
            continue;
        }
        if (!BoxOverlap(bl.cx, bl.cy, kBulletHalfW, kBulletHalfH, playerX, playerY, kPlayerHalfW, kPlayerHalfH)) {
            continue;
        }
        DeactivateBullet(bl);
        if (playerHurtCooldown > 0.0F) {
            continue;
        }
        playerHurtCooldown = kPlayerHurtCooldownSeconds;
        if (playerCharFsm != nullptr) {
            playerCharFsm->RequestHurt();
        }
        if (playerDamageable != nullptr) {
            playerDamageable->ApplyDamage(1.0F, nullptr);
        }
    }
}

void Platformer2DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        for (std::size_t gi = 0; gi < gemObjects.GetSize(); ++gi) {
            if (gemObjects[gi] != nullptr) {
                w.DestroyGameObject(gemObjects[gi]);
            }
        }
        gemObjects.Clear();
        gemsCollected = 0;
        gemsTotal = kGemCount;
        goalReached = false;
        facingLeft = false;
        sceneTime = 0.0F;
        enemiesDefeated = 0;
        playerHurtCooldown = 0.0F;
        enemies.Clear();
        enemyBullets.Clear();
        playerBaseScaleX = kPlayerHalfW * 2.0F;
        playerBaseScaleY = kPlayerHalfH * 2.0F;

        platformTilesTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("PlatTilesheet"));
        platformUsingKenneyTilesheet = TryLoadKenneySimplifiedPlatformerTilesheet(*platformTilesTex);
        if (!platformUsingKenneyTilesheet) {
            *platformTilesTex = Spark::Texture2D::CreateCheckerboard(
                    256,
                    32,
                    Spark::Vector3{0.42F, 0.36F, 0.30F},
                    Spark::Vector3{0.18F, 0.52F, 0.34F});
            platformTilesTex->GetName() = Spark::Utf8String("PlatCheckerFallback");
        }
        w.RegisterTexture(platformTilesTex, "spark/plat/kenney_simplified_tilesheet");

        playerAtlasTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("PlatPlayerAtlas"));
        playerUsingKenneyAtlas = TryBuildKenneyPlayerAtlas(*playerAtlasTex, playerAtlasColumns);
        if (!playerUsingKenneyAtlas) {
            *playerAtlasTex = MakePlayerRunAtlasFallback();
            playerAtlasColumns = 5U;
        }
        w.RegisterTexture(playerAtlasTex, "spark/plat/player_atlas");

        {
            const Spark::DemoAssets::PlatformerEnemyAtlasResult enemyAtlas = BuildPlatformerEnemyAtlas();
            enemyAtlasTex = Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(enemyAtlas.texture));
            enemyAtlasColumns = enemyAtlas.columns;
            enemyUsingKenneySlime = enemyAtlas.fromKenneySlime;
            enemyUsingTinyDungeon = enemyAtlas.fromTinyDungeon;
            enemyIdleUv = SpriteAnimatorComponent::ComputeUniformGridUv(enemyAtlasColumns, 1U, 0U);
            enemyAttackUv = enemyAtlasColumns >= 2U
                    ? SpriteAnimatorComponent::ComputeUniformGridUv(enemyAtlasColumns, 1U, 1U)
                    : enemyIdleUv;
            w.RegisterTexture(enemyAtlasTex, "spark/plat/enemy_atlas");
        }

        enemyBulletTex = Spark::MakeShared<Spark::Texture2D>(MakeEnemyBulletTextureFallback());
        w.RegisterTexture(enemyBulletTex, "spark/plat/enemy_bullet");

        gemTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("PlatGem"));
        gemUsingKenneyPng = TryLoadKenneyGemCollectible(*gemTex);
        if (!gemUsingKenneyPng) {
            *gemTex = MakeGemTextureFallback();
            gemTex->GetName() = Spark::Utf8String("PlatGemFallback");
        }
        w.RegisterTexture(gemTex, "spark/plat/gem_collectible");

        for (int i = 0; i < kPlatformCount; ++i) {
            const float x0 = kPlatforms[i][0];
            const float y0 = kPlatforms[i][1];
            const float x1 = kPlatforms[i][2];
            const float y1 = kPlatforms[i][3];
            const float cx = (x0 + x1) * 0.5F;
            const float cy = (y0 + y1) * 0.5F;
            const float sx = std::abs(x1 - x0);
            const float sy = std::abs(y1 - y0);
            Spark::GameObject* go = w.CreateGameObject();
            go->GetName() = Spark::Utf8String("Plat");
            Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({cx, cy, 0.01F + 0.001F * static_cast<float>(i)});
            tr->SetScale({sx, sy, 1.0F});
            go->AddComponent<Spark::SpriteComponent>(
                    platformTilesTex,
                    Spark::Vector4{0.95F, 0.92F, 0.88F, 1.0F},
                    platformUsingKenneyTilesheet
                            ? KenneySimplifiedPlatformerTileUv(kPlatformTileNumbers[static_cast<std::size_t>(i)])
                            : Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                    40 + i);
            go->AddComponent<Spark::BoxCollider2DComponent>();
            if (i == 10) {
                go->AddComponent<Spark::PhysicsMaterial2DComponent>(0.08F, 0.05F);
            }
            roots.PushBack(go);
        }

        playerObject = w.CreateGameObject();
        playerObject->GetName() = Spark::Utf8String("Player");
        playerTr = playerObject->AddComponent<Spark::TransformComponent>();
        playerTr->SetScale({playerBaseScaleX, playerBaseScaleY, 1.0F});
        playerCharFsm = playerObject->AddComponent<Spark::Sprite2DCharacterAnimFsmComponent>();
        playerObject->AddComponent<Spark::SpriteComponent>(
                playerAtlasTex,
                Spark::Vector4{0.98F, 0.95F, 0.92F, 1.0F},
                SpriteAnimatorComponent::ComputeUniformGridUv(playerAtlasColumns, kPlayerAtlasRows, 0),
                500);
        playerAnim = playerObject->AddComponent<Spark::SpriteAnimatorComponent>();
        playerAnim->SetUniformGrid(playerAtlasColumns, kPlayerAtlasRows);
        playerAnim->AddClip(SpriteAnimationClip{0, 1, 1.0F, true});
        playerAnim->AddClip(SpriteAnimationClip{1, 2, 10.0F, true});
        playerAnim->AddClip(SpriteAnimationClip{1, 1, 18.0F, false});
        playerAnim->AddClip(SpriteAnimationClip{2, 1, 14.0F, false});
        playerAnim->SetClipIndex(0);
        playerCharFsm->SetLocomotionClips(0, 1);
        playerCharFsm->SetCombatClips(2, 3);
        playerCharFsm->SetLocomotionSource(Sprite2DAnimLocomotionSource::SpeedSq);
        playerCharFsm->SetMoveSpeedThreshold(0.35F);
        playerObject->AddComponent<Spark::BoxCollider2DComponent>();
        playerRb = playerObject->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);
        playerHealth = playerObject->AddComponent<Spark::HealthComponent>(3.0F);
        playerDamageable = playerObject->AddComponent<Spark::DamageableComponent>();
        playerObject->AddComponent<Spark::SoundCueComponent>();

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("PlatFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*fpsText, false);
        {
            const char* plat = platformUsingKenneyTilesheet ? "Kenney tilesheet" : "checker fallback";
            const char* src = playerUsingKenneyAtlas
                    ? (playerAtlasColumns >= 5U
                               ? "Kenney character (idle, walk, happy=attack, duck=hurt)"
                               : "Kenney character (idle+walk only; add happy+duck PNGs for J/K)")
                    : "procedural 5-cell atlas (orange=attack, blue=hurt)";
            const char* enemySrc = enemyUsingKenneySlime ? "Kenney slime enemy"
                    : (enemyUsingTinyDungeon ? "Tiny Dungeon ghost enemy" : "procedural ghost enemy");
            fpsText->SetText(Spark::Utf8String(
                    std::format(
                            "Platformer — {} — {} — {} — WASD · Space · J attack · dodge enemy shots",
                            plat,
                            src,
                            enemySrc)
                            .c_str()));
        }
        roots.PushBack(playerObject);
        roots.PushBack(fpsHudObject);

        for (int gi = 0; gi < kGemCount; ++gi) {
            Spark::GameObject* gem = w.CreateGameObject();
            gem->GetName() = Spark::Utf8String("PlatGem");
            Spark::TransformComponent* gtr = gem->AddComponent<Spark::TransformComponent>();
            gtr->SetTranslation(
                    {kGemSpawns[static_cast<std::size_t>(gi)][0],
                     kGemSpawns[static_cast<std::size_t>(gi)][1],
                     0.05F + 0.0003F * static_cast<float>(gi)});
            gtr->SetScale({kGemDrawScale, kGemDrawScale, 1.0F});
            gem->AddComponent<Spark::SpriteComponent>(
                    gemTex,
                    Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                    Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                    620 + gi);
            const float hue = static_cast<float>(gi) * 0.51F;
            const Spark::Vector3 rgb{
                    0.42F + 0.5F * std::fabs(std::sin(hue)),
                    0.48F + 0.45F * std::fabs(std::sin(hue + 2.05F)),
                    0.72F + 0.28F * std::fabs(std::sin(hue + 4.1F))};
            const float pulseHz = 0.95F + 0.14F * static_cast<float>(gi % 7);
            const float emitStr = 1.45F + 0.12F * static_cast<float>(gi % 5);
            gem->AddComponent<Spark::SpriteLighting2DComponent>(
                    SpriteLighting2DMode::PulseEmission,
                    Spark::Vector4{rgb.x * 1.25F, rgb.y * 1.22F, rgb.z * 1.18F, pulseHz},
                    Spark::Vector4{emitStr, 0.48F, 0.0F, 0.0F});
            Spark::CircleCollider2DComponent* gemHit = gem->AddComponent<Spark::CircleCollider2DComponent>(1.0F);
            gemHit->SetIsTrigger(true);
            gemHit->SetCategoryBits(kGemHurtboxCategoryBits);
            gemHit->SetMaskBits(Spark::CollisionFilter2D::AllLayersMask());
            gem->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Static, 0.0F);
            gemObjects.PushBack(gem);
        }

        enemies.Clear();
        enemies.Resize(static_cast<std::size_t>(kEnemyCount));
        for (int ei = 0; ei < kEnemyCount; ++ei) {
            const float ex = kEnemySpawns[static_cast<std::size_t>(ei)][0];
            const float ey = kEnemySpawns[static_cast<std::size_t>(ei)][1];
            const float pMin = kEnemySpawns[static_cast<std::size_t>(ei)][2];
            const float pMax = kEnemySpawns[static_cast<std::size_t>(ei)][3];
            Spark::GameObject* ego = w.CreateGameObject();
            ego->GetName() = Spark::Utf8String("PlatEnemy");
            Spark::TransformComponent* etr = ego->AddComponent<Spark::TransformComponent>();
            etr->SetTranslation({ex, ey, 0.045F + 0.0002F * static_cast<float>(ei)});
            etr->SetScale({kEnemyDrawScale, kEnemyDrawScale, 1.0F});
            Spark::SpriteComponent* espr = ego->AddComponent<Spark::SpriteComponent>(
                    enemyAtlasTex,
                    Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                    enemyIdleUv,
                    710 + ei);
            Spark::CircleCollider2DComponent* enemyHit = ego->AddComponent<Spark::CircleCollider2DComponent>(0.68F);
            enemyHit->SetIsTrigger(true);
            enemyHit->SetCategoryBits(kEnemyHurtboxCategoryBits);
            enemyHit->SetMaskBits(Spark::CollisionFilter2D::AllLayersMask());
            ego->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Static, 0.0F);
            enemies[static_cast<std::size_t>(ei)] = {
                    true,
                    ego,
                    etr,
                    espr,
                    pMin,
                    pMax,
                    1.0F,
                    0.85F + 0.4F * static_cast<float>(ei),
                    0.0F,
                    1.7F * static_cast<float>(ei),
                    ey};
        }

        enemyBullets.Clear();
        enemyBullets.Resize(static_cast<std::size_t>(kMaxEnemyBullets));
        for (int bi = 0; bi < kMaxEnemyBullets; ++bi) {
            Spark::GameObject* bgo = w.CreateGameObject();
            bgo->GetName() = Spark::Utf8String("PlatEnemyBullet");
            Spark::TransformComponent* btr = bgo->AddComponent<Spark::TransformComponent>();
            btr->SetUniformScale(kBulletDrawScale);
            btr->SetTranslation({-120.0F, -120.0F, 0.0F});
            bgo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
            Spark::SpriteComponent* bspr = bgo->AddComponent<Spark::SpriteComponent>(
                    enemyBulletTex,
                    Spark::Vector4{1.0F, 0.72F, 0.38F, 0.0F},
                    Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                    760 + bi);
            roots.PushBack(bgo);
            enemyBullets[static_cast<std::size_t>(bi)] = {
                    false, bgo, btr, bspr, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
        }

        const float spawnY = kGroundSurfaceY + kPlayerHalfH;
        playerTr->SetTranslation({kPlayerSpawnX, spawnY, 0.04F});
        playerRb->SetVelocity(Spark::Vector2::Zero);

        mainCameraGo = w.CreateGameObject();
        mainCameraGo->GetName() = Spark::Utf8String("MainCamera");
        roots.PushBack(mainCameraGo);
        Spark::TransformComponent* camTr = mainCameraGo->AddComponent<Spark::TransformComponent>();
        camTr->SetTranslation({kPlayerSpawnX, spawnY + 1.2F, 0.0F});
        Spark::Camera2DComponent* cam = mainCameraGo->AddComponent<Spark::Camera2DComponent>();
        cam->SetHalfExtentY(8.5F);
        cam->SetPriority(10);
        cameraRig = mainCameraGo->AddComponent<Spark::Camera2DRigComponent>();
        cameraRig->SetMode(Spark::Camera2DRigMode::BoundedFollow);
        cameraRig->SetTarget(playerObject);
        cameraRig->SetTargetOffset({0.0F, 1.48F, 0.0F});
        cameraRig->SetFollowSmoothRate(7.5F);
        cameraRig->SetUseBounds(true);
        cameraRig->SetBoundsMin({-8.0F, -1.5F});
        cameraRig->SetBoundsMax({50.0F, 9.0F});

        context.GetInput().SetCursorCaptured(false);
    }

void Platformer2DDemo::Unload(Spark::GameWorld& w)
{
        for (std::size_t gi = 0; gi < gemObjects.GetSize(); ++gi) {
            if (gemObjects[gi] != nullptr) {
                w.DestroyGameObject(gemObjects[gi]);
            }
        }
        gemObjects.Clear();
        for (std::size_t ei = 0; ei < enemies.GetSize(); ++ei) {
            if (enemies[ei].go != nullptr) {
                w.DestroyGameObject(enemies[ei].go);
            }
        }
        enemies.Clear();
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        gemTex.Reset();
        platformTilesTex.Reset();
        playerAtlasTex.Reset();
        enemyAtlasTex.Reset();
        enemyBulletTex.Reset();
        playerObject = nullptr;
        playerTr = nullptr;
        playerRb = nullptr;
        playerHealth = nullptr;
        playerDamageable = nullptr;
        playerAnim = nullptr;
        playerCharFsm = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
        mainCameraGo = nullptr;
        cameraRig = nullptr;
        attackArcHitsScratch.Clear();
        enemies.Clear();
        enemyBullets.Clear();
        enemiesDefeated = 0;
        playerHurtCooldown = 0.0F;
    }

void Platformer2DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world)
{
        sceneTime += timing.deltaTimeSeconds;
        Spark::IInput& in = context.GetInput();
        const float dt = timing.deltaTimeSeconds;
        if (playerHurtCooldown > 0.0F) {
            playerHurtCooldown = std::max(0.0F, playerHurtCooldown - dt);
        }

        if (playerRb != nullptr && playerTr != nullptr) {
            float run = 0.0F;
            if (in.IsKeyDown(GLFW_KEY_A) || in.IsKeyDown(GLFW_KEY_LEFT)) {
                run -= 1.0F;
            }
            if (in.IsKeyDown(GLFW_KEY_D) || in.IsKeyDown(GLFW_KEY_RIGHT)) {
                run += 1.0F;
            }
            if (playerCharFsm != nullptr) {
                if (in.IsKeyPressedThisFrame(GLFW_KEY_J)) {
                    playerCharFsm->RequestAttack();
                }
                if (in.IsKeyPressedThisFrame(GLFW_KEY_K)) {
                    playerCharFsm->RequestHurt();
                    if (playerDamageable != nullptr) {
                        playerDamageable->ApplyDamage(1.0F, playerObject);
                    }
                }
            }
            if (std::abs(run) > 0.5F) {
                facingLeft = (run < 0.0F);
            }
            playerTr->SetScale(
                    {facingLeft ? -playerBaseScaleX : playerBaseScaleX, playerBaseScaleY, 1.0F});

            Spark::Vector2 v = playerRb->GetVelocity();
            v.x = run * 10.0F;
            if (playerRb->IsGrounded() && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
                v.y = 12.8F;
            }
            playerRb->SetVelocity(v);

            Spark::PhysicsWorld2DSettings phys{};
            phys.gravityY = -32.0F;
            phys.maxFallSpeed = 46.0F;
            Spark::SimulatePhysics2D(world, timing, phys);

            const Spark::Vector3 p = playerTr->GetLocalTransform().translation;
            const bool attackArcActive = playerAnim != nullptr && playerAnim->GetClipIndex() == kPlayerAttackClipIndex &&
                    !playerAnim->IsCurrentClipFinished();
            if (attackArcActive) {
                const float dirX = facingLeft ? -1.0F : 1.0F;
                constexpr float dirY = 0.0F;
                const float originX = p.x + dirX * (kPlayerHalfW * 0.55F);
                const float originY = p.y + kPlayerHalfH * 0.12F;
                PhysicsQueryFilter2D weaponFilter{};
                weaponFilter.queryCategoryBits = CollisionFilter2D::LayerBit(2);
                weaponFilter.queryMaskBits = kGemHurtboxCategoryBits | kEnemyHurtboxCategoryBits;
                weaponFilter.hitSolids = false;
                weaponFilter.hitTriggers = true;
                StaticBroadPhase2D combatBp;
                combatBp.Rebuild(world, 4.0F);
                QueryOverlapArcStatics2D(
                        combatBp,
                        originX,
                        originY,
                        kAttackArcRadius,
                        dirX,
                        dirY,
                        kAttackArcHalfAngleRad,
                        weaponFilter,
                        attackArcHitsScratch);
                for (std::size_t hi = 0; hi < attackArcHitsScratch.GetSize(); ++hi) {
                    Spark::GameObject* hitOwner = attackArcHitsScratch[hi].owner;
                    if (hitOwner == nullptr) {
                        continue;
                    }
                    bool handled = false;
                    for (std::size_t gi = 0; gi < gemObjects.GetSize(); ++gi) {
                        if (gemObjects[gi] != hitOwner) {
                            continue;
                        }
                        world.DestroyGameObject(hitOwner);
                        gemObjects.RemoveAt(gi);
                        ++gemsCollected;
                        DemoAudio::QueueCue(*playerObject, DemoSfx::ClipGemCollect(), 0.95F);
                        handled = true;
                        break;
                    }
                    if (handled) {
                        continue;
                    }
                    for (std::size_t ei = 0; ei < enemies.GetSize(); ++ei) {
                        EnemySlot& enemy = enemies[ei];
                        if (!enemy.alive || enemy.go != hitOwner) {
                            continue;
                        }
                        enemy.alive = false;
                        world.DestroyGameObject(enemy.go);
                        enemy.go = nullptr;
                        enemy.tr = nullptr;
                        enemy.spr = nullptr;
                        ++enemiesDefeated;
                        DemoAudio::QueueCue(*playerObject, DemoSfx::ClipInvadersHit(), 0.9F);
                        break;
                    }
                }
            }

            UpdateEnemies(dt, p.x, p.y);
            UpdateEnemyBullets(dt);
            ResolveEnemyBulletHits(p.x, p.y);
            if (p.y < kFallRespawnY) {
                for (std::size_t bi = 0; bi < enemyBullets.GetSize(); ++bi) {
                    DeactivateBullet(enemyBullets[bi]);
                }
                playerHurtCooldown = 0.0F;
                if (playerDamageable != nullptr) {
                    playerDamageable->ApplyDamage(1.0F, nullptr);
                    if (playerHealth != nullptr && !playerHealth->IsAlive()) {
                        playerHealth->ResetToFull();
                    }
                }
                playerTr->SetTranslation({kPlayerSpawnX, kGroundSurfaceY + kPlayerHalfH, p.z});
                playerRb->SetVelocity(Spark::Vector2::Zero);
                goalReached = false;
            }

            const float gcr2 = kGemCollectRadius * kGemCollectRadius;
            for (std::size_t gi = 0; gi < gemObjects.GetSize();) {
                Spark::GameObject* gem = gemObjects[gi];
                if (gem == nullptr) {
                    gemObjects.RemoveAt(gi);
                    continue;
                }
                const Spark::TransformComponent* gtr = gem->GetComponent<Spark::TransformComponent>();
                if (gtr == nullptr) {
                    world.DestroyGameObject(gem);
                    gemObjects.RemoveAt(gi);
                    continue;
                }
                const Spark::Vector3 gpos = gtr->GetLocalTransform().translation;
                const float gdx = gpos.x - p.x;
                const float gdy = gpos.y - p.y;
                if (gdx * gdx + gdy * gdy <= gcr2) {
                    world.DestroyGameObject(gem);
                    gemObjects.RemoveAt(gi);
                    ++gemsCollected;
                    DemoAudio::QueueCue(*playerObject, DemoSfx::ClipGemCollect(), 0.95F);
                    continue;
                }
                ++gi;
            }

            if (!goalReached && p.x > 39.0F && p.x < 47.5F && p.y > 6.6F && p.y < 8.4F) {
                goalReached = true;
            }
        }

        if (cameraRig != nullptr && mainCameraGo != nullptr) {
            int fbW = 0;
            int fbH = 0;
            context.GetFramebufferSize(fbW, fbH);
            float aspect = 1.0F;
            if (fbH > 0) {
                aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
            }
            Spark::Camera2DRigComponent::Tick(
                    *cameraRig,
                    *mainCameraGo,
                    dt,
                    aspect);
        }

        if (fpsText != nullptr) {
            const float tdt = timing.deltaTimeSeconds;
            const float instant = (tdt > 1.0e-6F) ? (1.0F / tdt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            std::string hud = std::format(
                    "Platformer — {:.0f} FPS — HP {:.0f} — gems {}/{} — enemies {}/{} — WASD · Space · J attack",
                    static_cast<double>(fpsSmoothed),
                    playerHealth != nullptr ? static_cast<double>(playerHealth->GetCurrent()) : 0.0,
                    gemsCollected,
                    gemsTotal,
                    enemiesDefeated,
                    kEnemyCount);
            if (!playerUsingKenneyAtlas || !platformUsingKenneyTilesheet || !gemUsingKenneyPng) {
                hud += " — [asset fallback]";
            }
            if (goalReached) {
                hud += " — Summit!";
            }
            fpsText->SetText(Spark::Utf8String(hud.c_str()));
        }
    }

void Platformer2DDemo::Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        (void)Spark::SubmitStandardLitSceneFromWorldWithCamera(
                world,
                context,
                Spark::Vector3{0.28F, 0.88F, 0.38F}.Normalized(),
                Spark::Vector3{1.0F, 1.0F, 1.0F},
                0.9F,
                Spark::Vector3{0.18F, 0.20F, 0.26F},
                false,
                sceneTime);
    }
}  // namespace Spark
