#include "spark/demo/Platformer2DDemo.hpp"
#include "spark/demo/Platformer2DDemo_detail.hpp"
#include "spark/demo/DemoFoundation.hpp"
#include "spark/audio/SoundFileLoader.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/ecs/components/gameplay/DamageableComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/physics/2d/PhysicsMaterial2DComponent.hpp"
#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/components/camera/Camera2DComponent.hpp"
#include "spark/ecs/components/camera/Camera2DRigComponent.hpp"
#include "spark/scene/SceneSubmit.hpp"

namespace Spark {

namespace {

constexpr std::uint32_t kPlayerAtlasRows = 1U;

Spark::SharedPtr<Spark::Texture2D> MakeHudWhitePixelTexture()
{
    Spark::Texture2D tex(Spark::Utf8String("PlatHudWhitePixel"));
    Spark::Array<std::uint8_t> px;
    px.Resize(4U);
    px[0] = 255;
    px[1] = 255;
    px[2] = 255;
    px[3] = 255;
    tex.SetPixels(1U, 1U, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

Spark::SharedPtr<Spark::SoundClip> LoadPlatformerSfx(const char* bundledAssetPath)
{
    return TryLoadSoundClipFromBundledAsset(bundledAssetPath);
}

Spark::SharedPtr<Spark::SoundClip> LoadPlatformerBgm()
{
    Spark::SharedPtr<Spark::SoundClip> clip =
            TryLoadSoundClipFromBundledAsset("assets/audio/time_for_adventure.wav");
    if (!clip) {
        clip = TryLoadSoundClipFromBundledAsset("assets/audio/time_for_adventure.mp3");
    }
    return clip;
}

}  // namespace

Platformer2D::BulletProfile Platformer2DDemo::MakePlayerBulletProfile() const noexcept
{
    Platformer2D::BulletProfile profile{};
    profile.speed = Platformer2D::Config::kPlayerBulletSpeed;
    profile.halfW = Platformer2D::Config::kPlayerBulletHalfW;
    profile.halfH = Platformer2D::Config::kPlayerBulletHalfH;
    profile.drawScale = Platformer2D::Config::kPlayerBulletDrawScale;
    profile.lifetime = Platformer2D::Config::kBulletLifetimeSeconds;
    profile.baseTint = {0.35F, 0.88F, 1.0F, 0.94F};
    profile.additiveBlend = true;
    return profile;
}

Platformer2D::BulletProfile Platformer2DDemo::MakeEnemyBulletProfile() const noexcept
{
    Platformer2D::BulletProfile profile{};
    profile.speed = Platformer2D::Config::kEnemyBulletSpeed;
    profile.halfW = Platformer2D::Config::kEnemyBulletHalfW;
    profile.halfH = Platformer2D::Config::kEnemyBulletHalfH;
    profile.drawScale = Platformer2D::Config::kEnemyBulletDrawScale;
    profile.lifetime = Platformer2D::Config::kBulletLifetimeSeconds;
    profile.baseTint = {1.0F, 0.62F, 0.28F, 0.90F};
    profile.additiveBlend = true;
    return profile;
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
    enemySquad.Unload(w);
    playerBullets.Shutdown(w);
    enemyBullets.Shutdown(w);
    explosions.Shutdown(w);
    healthHud.Shutdown(w);

    gemsCollected = 0;
    gemsTotal = 0;
    goalReached = false;
    sceneTime = 0.0F;

    PhysicsWorld2DSettings& phys = physics.GetWorld2D().GetSettings();
    phys.gravityY = -32.0F;
    phys.maxFallSpeed = 46.0F;
    phys.resolveDynamicVsDynamic = false;
    phys.jointIterations = 4;
    gemsTotal = kGemCount;
    goalReached = false;
    facingLeft = false;
    sceneTime = 0.0F;
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
    if (!TryBuildKenneyPlayerAtlas(*playerAtlasTex, playerAtlasColumns)) {
        *playerAtlasTex = MakePlayerRunAtlasFallback();
        playerAtlasColumns = 5U;
    }
    w.RegisterTexture(playerAtlasTex, "spark/plat/player_atlas");

    {
        const Spark::DemoAssets::PlatformerEnemyAtlasResult enemyAtlas = BuildPlatformerEnemyAtlas();
        enemyAtlasTex = Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(enemyAtlas.texture));
        enemyAtlasColumns = enemyAtlas.columns;
        enemyIdleUv = SpriteAnimatorComponent::ComputeUniformGridUv(enemyAtlasColumns, 1U, 0U);
        enemyAttackUv = enemyAtlasColumns >= 2U
                ? SpriteAnimatorComponent::ComputeUniformGridUv(enemyAtlasColumns, 1U, 1U)
                : enemyIdleUv;
        w.RegisterTexture(enemyAtlasTex, "spark/plat/enemy_atlas");
    }

    playerBulletTex = Spark::MakeShared<Spark::Texture2D>(MakePlayerBulletTextureFallback());
    enemyBulletTex = Spark::MakeShared<Spark::Texture2D>(MakeEnemyBulletTextureFallback());
    hudWhiteTex = MakeHudWhitePixelTexture();
    w.RegisterTexture(playerBulletTex, "spark/plat/player_bullet");
    w.RegisterTexture(enemyBulletTex, "spark/plat/enemy_bullet");
    w.RegisterTexture(hudWhiteTex, "spark/plat/hud_white");

    gemTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("PlatGem"));
    if (!TryLoadKenneyGemCollectible(*gemTex)) {
        *gemTex = MakeGemTextureFallback();
        gemTex->GetName() = Spark::Utf8String("PlatGemFallback");
    }
    w.RegisterTexture(gemTex, "spark/plat/gem_collectible");

    sfxJump = LoadPlatformerSfx("assets/audio/jump.wav");
    sfxCoin = LoadPlatformerSfx("assets/audio/coin.wav");
    sfxExplosion = LoadPlatformerSfx("assets/audio/explosion.wav");
    sfxHurt = LoadPlatformerSfx("assets/audio/hurt.wav");

    if (audioEngine != nullptr) {
        audioEngine->ClearBackgroundMusic();
        audioEngine = nullptr;
    }
    audioEngine = context.TryGetSoundEngine();
    if (audioEngine != nullptr && audioEngine->IsRunning()) {
        if (Spark::SharedPtr<Spark::SoundClip> bgm = LoadPlatformerBgm()) {
            audioEngine->SetBackgroundMusic(bgm, 0.28F, true);
        }
    }

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
        roots.Track(go);
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
    playerHealth = playerObject->AddComponent<Spark::HealthComponent>(Platformer2D::Config::kPlayerMaxHealth);
    playerDamageable = playerObject->AddComponent<Spark::DamageableComponent>();
    playerObject->AddComponent<Spark::SoundCueComponent>();
    roots.Track(playerObject);

    healthHud.Initialize(w, hudWhiteTex, roots);

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
        gem->AddComponent<Spark::SpriteLighting2DComponent>(
                SpriteLighting2DMode::PulseEmission,
                Spark::Vector4{rgb.x * 1.25F, rgb.y * 1.22F, rgb.z * 1.18F, 0.95F + 0.14F * static_cast<float>(gi % 7)},
                Spark::Vector4{1.45F + 0.12F * static_cast<float>(gi % 5), 0.48F, 0.0F, 0.0F});
        Spark::CircleCollider2DComponent* gemHit = gem->AddComponent<Spark::CircleCollider2DComponent>(1.0F);
        gemHit->SetIsTrigger(true);
        gemHit->SetCategoryBits(kGemHurtboxCategoryBits);
        gemHit->SetMaskBits(Spark::CollisionFilter2D::AllLayersMask());
        gem->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Static, 0.0F);
        gemObjects.PushBack(gem);
    }

    enemySquad.Load(w, enemyAtlasTex, enemyIdleUv, enemyAttackUv);

    playerBullets.Initialize(
            w,
            playerBulletTex,
            Platformer2D::Config::kMaxPlayerBullets,
            740,
            Spark::Utf8String("PlatPlayerBullet"),
            roots);
    enemyBullets.Initialize(
            w,
            enemyBulletTex,
            Platformer2D::Config::kMaxEnemyBullets,
            760,
            Spark::Utf8String("PlatEnemyBullet"),
            roots);
    explosions.Initialize(w, enemyBulletTex, Platformer2D::Config::kExplosionMaxParticles, 9000, roots);

    const float spawnY = kGroundSurfaceY + kPlayerHalfH;
    playerTr->SetTranslation({kPlayerSpawnX, spawnY, 0.04F});
    playerRb->SetVelocity(Spark::Vector2::Zero);
    healthHud.SetHealth(Platformer2D::Config::kPlayerMaxHealth, Platformer2D::Config::kPlayerMaxHealth);

    mainCameraGo = w.CreateGameObject();
    mainCameraGo->GetName() = Spark::Utf8String("MainCamera");
    roots.Track(mainCameraGo);
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
    if (audioEngine != nullptr) {
        audioEngine->ClearBackgroundMusic();
        audioEngine = nullptr;
    }
    for (std::size_t gi = 0; gi < gemObjects.GetSize(); ++gi) {
        if (gemObjects[gi] != nullptr) {
            w.DestroyGameObject(gemObjects[gi]);
        }
    }
    gemObjects.Clear();
    enemySquad.Unload(w);
    playerBullets.Shutdown(w);
    enemyBullets.Shutdown(w);
    explosions.Shutdown(w);
    healthHud.Shutdown(w);
    roots.DestroyAll(w);

    gemTex.Reset();
    platformTilesTex.Reset();
    playerAtlasTex.Reset();
    enemyAtlasTex.Reset();
    playerBulletTex.Reset();
    enemyBulletTex.Reset();
    hudWhiteTex.Reset();
    sfxJump.Reset();
    sfxCoin.Reset();
    sfxExplosion.Reset();
    sfxHurt.Reset();
    playerObject = nullptr;
    playerTr = nullptr;
    playerRb = nullptr;
    playerHealth = nullptr;
    playerDamageable = nullptr;
    playerAnim = nullptr;
    playerCharFsm = nullptr;
    mainCameraGo = nullptr;
    cameraRig = nullptr;
}

void Platformer2DDemo::Simulate(
        const Spark::FrameTiming& timing,
        Spark::IEngineContext& context,
        Spark::GameWorld& world)
{
    sceneTime += timing.deltaTimeSeconds;
    Spark::IInput& in = context.GetInput();
    const float dt = timing.deltaTimeSeconds;
    const Platformer2D::BulletProfile playerBulletProfile = MakePlayerBulletProfile();
    const Platformer2D::BulletProfile enemyBulletProfile = MakeEnemyBulletProfile();

    playerCombat.TickCooldown(dt);
    explosions.Tick(dt);

    if (playerRb != nullptr && playerTr != nullptr) {
        float run = 0.0F;
        if (in.IsKeyDown(GLFW_KEY_A) || in.IsKeyDown(GLFW_KEY_LEFT)) {
            run -= 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_D) || in.IsKeyDown(GLFW_KEY_RIGHT)) {
            run += 1.0F;
        }
        const bool attackPressed = in.IsKeyPressedThisFrame(GLFW_KEY_J);
        if (playerCharFsm != nullptr && attackPressed) {
            playerCharFsm->RequestAttack();
        }
        if (std::abs(run) > 0.5F) {
            facingLeft = (run < 0.0F);
        }
        playerTr->SetScale({facingLeft ? -playerBaseScaleX : playerBaseScaleX, playerBaseScaleY, 1.0F});

        Spark::Vector2 v = playerRb->GetVelocity();
        v.x = run * 10.0F;
        const bool jumpPressed = playerRb->IsGrounded() && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE);
        if (jumpPressed) {
            v.y = 12.8F;
            if (playerObject != nullptr && sfxJump.Get() != nullptr) {
                DemoAudio::QueueCue(*playerObject, sfxJump, 0.95F);
            }
        }
        playerRb->SetVelocity(v);

        physics.Simulate2D(world, timing);

        const Spark::Vector3 p = playerTr->GetLocalTransform().translation;
        playerCombat.TryFireOnAttackPressed(
                attackPressed,
                p.x,
                p.y,
                facingLeft,
                playerBullets,
                playerBulletProfile);

        enemySquad.Tick(dt, sceneTime, p.x, p.y, enemyBullets, enemyBulletProfile);
        playerBullets.Tick(dt, -18.0F, 54.0F, -14.0F, 12.0F);
        enemyBullets.Tick(dt, -18.0F, 54.0F, -14.0F, 12.0F);

        const int killed = enemySquad.ResolvePlayerBulletHits(playerBullets, explosions, world);
        if (killed > 0 && playerObject != nullptr && sfxExplosion.Get() != nullptr) {
            DemoAudio::QueueCue(*playerObject, sfxExplosion, 0.92F);
        }

        playerCombat.ResolveEnemyBulletHits(
                enemyBullets,
                p.x,
                p.y,
                playerHealth,
                playerDamageable,
                playerCharFsm,
                playerObject,
                sfxHurt);
        if (playerHealth != nullptr && !playerHealth->IsAlive()) {
            playerHealth->ResetToFull();
            playerTr->SetTranslation({kPlayerSpawnX, kGroundSurfaceY + kPlayerHalfH, p.z});
            playerRb->SetVelocity(Spark::Vector2::Zero);
            playerCombat.ClearIncomingProjectiles(enemyBullets);
            goalReached = false;
        }

        if (p.y < kFallRespawnY) {
            playerCombat.ClearIncomingProjectiles(enemyBullets);
            if (playerDamageable != nullptr) {
                playerDamageable->ApplyDamage(Platformer2D::Config::kFallDamage, nullptr);
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
                if (playerObject != nullptr && sfxCoin.Get() != nullptr) {
                    DemoAudio::QueueCue(*playerObject, sfxCoin, 0.95F);
                }
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
        Spark::Camera2DRigComponent::Tick(*cameraRig, *mainCameraGo, dt, aspect);
        if (Spark::Camera2DComponent* cam = mainCameraGo->GetComponent<Spark::Camera2DComponent>()) {
            healthHud.SyncToCamera(*cam, *mainCameraGo, static_cast<float>(fbW), static_cast<float>(fbH));
        }
    }

    if (playerHealth != nullptr) {
        healthHud.SetHealth(playerHealth->GetCurrent(), playerHealth->GetMaximum());
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
