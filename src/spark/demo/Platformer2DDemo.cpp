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
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/vfx/VfxSubsystemProcess.hpp"

#include <cstdio>

namespace Spark {

namespace {

constexpr std::uint32_t kPlayerAtlasRows = 1U;
constexpr float kBackgroundAnchorX = 20.0F;

Spark::SharedPtr<Spark::Texture2D> MakeSkyGradientTexture()
{
    constexpr std::uint32_t kW = 4U;
    constexpr std::uint32_t kH = 128U;
    Spark::Texture2D tex(Spark::Utf8String("PlatBgSky"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4U);
    for (std::uint32_t y = 0; y < kH; ++y) {
        const float t = static_cast<float>(y) / static_cast<float>(kH - 1U);
        const Spark::Vector3 top{0.10F, 0.16F, 0.42F};
        const Spark::Vector3 horizon{0.98F, 0.58F, 0.34F};
        const Spark::Vector3 rgb{
                top.x + (horizon.x - top.x) * std::pow(t, 1.35F),
                top.y + (horizon.y - top.y) * std::pow(t, 1.35F),
                top.z + (horizon.z - top.z) * std::pow(t, 1.35F)};
        for (std::uint32_t x = 0; x < kW; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * kW + x) * 4U;
            px[i] = static_cast<std::uint8_t>(std::clamp(rgb.x * 255.0F, 0.0F, 255.0F));
            px[i + 1U] = static_cast<std::uint8_t>(std::clamp(rgb.y * 255.0F, 0.0F, 255.0F));
            px[i + 2U] = static_cast<std::uint8_t>(std::clamp(rgb.z * 255.0F, 0.0F, 255.0F));
            px[i + 3U] = 255;
        }
    }
    tex.SetPixels(kW, kH, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

Spark::SharedPtr<Spark::Texture2D> MakeHillSilhouetteTexture(const bool farLayer)
{
    constexpr std::uint32_t kW = 256U;
    constexpr std::uint32_t kH = 64U;
    Spark::Texture2D tex(Spark::Utf8String(farLayer ? "PlatBgMountains" : "PlatBgHills"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4U);
    const Spark::Vector3 hillRgb = farLayer ? Spark::Vector3{0.18F, 0.22F, 0.34F} : Spark::Vector3{0.12F, 0.34F, 0.28F};
    for (std::uint32_t y = 0; y < kH; ++y) {
        for (std::uint32_t x = 0; x < kW; ++x) {
            const float nx = static_cast<float>(x) / static_cast<float>(kW);
            const float waveA = std::sin(nx * 6.283F * (farLayer ? 1.4F : 2.2F)) * (farLayer ? 0.10F : 0.14F);
            const float waveB = std::sin(nx * 6.283F * (farLayer ? 3.1F : 4.8F) + 1.2F) * (farLayer ? 0.05F : 0.08F);
            const float crest = (farLayer ? 0.42F : 0.28F) + waveA + waveB;
            const float ny = static_cast<float>(y) / static_cast<float>(kH);
            const bool inside = ny <= crest;
            const std::size_t i = (static_cast<std::size_t>(y) * kW + x) * 4U;
            px[i] = static_cast<std::uint8_t>(hillRgb.x * 255.0F);
            px[i + 1U] = static_cast<std::uint8_t>(hillRgb.y * 255.0F);
            px[i + 2U] = static_cast<std::uint8_t>(hillRgb.z * 255.0F);
            px[i + 3U] = inside ? 255 : 0;
        }
    }
    tex.SetPixels(kW, kH, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

Spark::SharedPtr<Spark::Texture2D> MakeCloudTexture()
{
    constexpr std::uint32_t kN = 48U;
    Spark::Texture2D tex(Spark::Utf8String("PlatBgCloud"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(kN) * static_cast<std::size_t>(kN) * 4U);
    for (std::uint32_t y = 0; y < kN; ++y) {
        for (std::uint32_t x = 0; x < kN; ++x) {
            const float fx = (static_cast<float>(x) + 0.5F) / static_cast<float>(kN) - 0.5F;
            const float fy = (static_cast<float>(y) + 0.5F) / static_cast<float>(kN) - 0.5F;
            const float d = std::sqrt(fx * fx * 1.8F + fy * fy);
            const float alpha = std::clamp(1.0F - d * 2.2F, 0.0F, 1.0F);
            const std::size_t i = (static_cast<std::size_t>(y) * kN + x) * 4U;
            px[i] = 255;
            px[i + 1U] = 255;
            px[i + 2U] = 255;
            px[i + 3U] = static_cast<std::uint8_t>(alpha * alpha * 190.0F);
        }
    }
    tex.SetPixels(kN, kN, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

Spark::SharedPtr<Spark::Texture2D> MakeGoalFlagTexture()
{
    constexpr std::uint32_t kW = 24U;
    constexpr std::uint32_t kH = 40U;
    Spark::Texture2D tex(Spark::Utf8String("PlatGoalFlag"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(kW) * static_cast<std::size_t>(kH) * 4U);
    for (std::uint32_t y = 0; y < kH; ++y) {
        for (std::uint32_t x = 0; x < kW; ++x) {
            const float nx = static_cast<float>(x) / static_cast<float>(kW);
            const float ny = static_cast<float>(y) / static_cast<float>(kH);
            bool inside = false;
            Spark::Vector3 rgb{0.0F, 0.0F, 0.0F};
            if (nx < 0.12F && ny > 0.05F) {
                inside = true;
                rgb = {0.42F, 0.30F, 0.18F};
            } else if (nx >= 0.12F && ny > 0.55F) {
                const int stripe = static_cast<int>((nx - 0.12F) * 14.0F) % 2;
                inside = true;
                rgb = stripe == 0 ? Spark::Vector3{0.95F, 0.18F, 0.22F} : Spark::Vector3{0.98F, 0.96F, 0.92F};
            } else if (nx >= 0.12F && ny > 0.38F) {
                inside = true;
                rgb = {0.95F, 0.82F, 0.18F};
            }
            const std::size_t i = (static_cast<std::size_t>(y) * kW + x) * 4U;
            px[i] = static_cast<std::uint8_t>(rgb.x * 255.0F);
            px[i + 1U] = static_cast<std::uint8_t>(rgb.y * 255.0F);
            px[i + 2U] = static_cast<std::uint8_t>(rgb.z * 255.0F);
            px[i + 3U] = inside ? 255 : 0;
        }
    }
    tex.SetPixels(kW, kH, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

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

void Platformer2DDemo::SpawnBackgroundLayers(Spark::GameWorld& world)
{
    bgSkyTex = MakeSkyGradientTexture();
    bgMountainsTex = MakeHillSilhouetteTexture(true);
    bgHillsTex = MakeHillSilhouetteTexture(false);
    bgCloudTex = MakeCloudTexture();
    goalFlagTex = MakeGoalFlagTexture();
    world.RegisterTexture(bgSkyTex, "spark/plat/bg_sky");
    world.RegisterTexture(bgMountainsTex, "spark/plat/bg_mountains");
    world.RegisterTexture(bgHillsTex, "spark/plat/bg_hills");
    world.RegisterTexture(bgCloudTex, "spark/plat/bg_cloud");
    world.RegisterTexture(goalFlagTex, "spark/plat/goal_flag");

    bgSkyGo = world.CreateGameObject();
    bgSkyGo->GetName() = Spark::Utf8String("PlatBgSky");
    bgSkyTr = bgSkyGo->AddComponent<Spark::TransformComponent>();
    bgSkyTr->SetTranslation({kBackgroundAnchorX, 4.2F, -0.45F});
    bgSkyTr->SetScale({120.0F, 22.0F, 1.0F});
    bgSkyGo->AddComponent<Spark::SpriteComponent>(
            bgSkyTex, Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F}, Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F}, 1);
    roots.Track(bgSkyGo);

    bgMountainsGo = world.CreateGameObject();
    bgMountainsGo->GetName() = Spark::Utf8String("PlatBgMountains");
    bgMountainsTr = bgMountainsGo->AddComponent<Spark::TransformComponent>();
    bgMountainsTr->SetTranslation({kBackgroundAnchorX, 1.35F, -0.35F});
    bgMountainsTr->SetScale({92.0F, 10.5F, 1.0F});
    bgMountainsGo->AddComponent<Spark::SpriteComponent>(
            bgMountainsTex,
            Spark::Vector4{0.88F, 0.90F, 0.98F, 0.92F},
            Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            2);
    roots.Track(bgMountainsGo);

    bgHillsGo = world.CreateGameObject();
    bgHillsGo->GetName() = Spark::Utf8String("PlatBgHills");
    bgHillsTr = bgHillsGo->AddComponent<Spark::TransformComponent>();
    bgHillsTr->SetTranslation({kBackgroundAnchorX, 0.55F, -0.25F});
    bgHillsTr->SetScale({88.0F, 8.5F, 1.0F});
    bgHillsGo->AddComponent<Spark::SpriteComponent>(
            bgHillsTex,
            Spark::Vector4{0.72F, 0.95F, 0.78F, 0.95F},
            Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            3);
    roots.Track(bgHillsGo);

    bgCloudGoA = world.CreateGameObject();
    bgCloudGoA->GetName() = Spark::Utf8String("PlatBgCloudA");
    bgCloudTrA = bgCloudGoA->AddComponent<Spark::TransformComponent>();
    bgCloudTrA->SetTranslation({6.0F, 6.8F, -0.15F});
    bgCloudTrA->SetScale({7.5F, 3.2F, 1.0F});
    bgCloudGoA->AddComponent<Spark::SpriteComponent>(
            bgCloudTex, Spark::Vector4{1.0F, 1.0F, 1.0F, 0.82F}, Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F}, 4);
    bgCloudGoA->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::AlphaOver);
    roots.Track(bgCloudGoA);

    bgCloudGoB = world.CreateGameObject();
    bgCloudGoB->GetName() = Spark::Utf8String("PlatBgCloudB");
    bgCloudTrB = bgCloudGoB->AddComponent<Spark::TransformComponent>();
    bgCloudTrB->SetTranslation({34.0F, 7.4F, -0.14F});
    bgCloudTrB->SetScale({9.0F, 3.8F, 1.0F});
    bgCloudGoB->AddComponent<Spark::SpriteComponent>(
            bgCloudTex, Spark::Vector4{1.0F, 1.0F, 1.0F, 0.74F}, Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F}, 5);
    bgCloudGoB->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::AlphaOver);
    roots.Track(bgCloudGoB);

    goalGlowGo = world.CreateGameObject();
    goalGlowGo->GetName() = Spark::Utf8String("PlatGoalGlow");
    goalGlowTr = goalGlowGo->AddComponent<Spark::TransformComponent>();
    goalGlowTr->SetTranslation({kGoalCenterX, kGoalCenterY + 0.35F, 0.02F});
    goalGlowTr->SetScale({2.4F, 2.4F, 1.0F});
    goalGlowGo->AddComponent<Spark::SpriteComponent>(
            hudWhiteTex,
            Spark::Vector4{1.0F, 0.88F, 0.35F, 0.35F},
            Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            15);
    goalGlowGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
    roots.Track(goalGlowGo);

    goalFlagGo = world.CreateGameObject();
    goalFlagGo->GetName() = Spark::Utf8String("PlatGoalFlag");
    goalFlagTr = goalFlagGo->AddComponent<Spark::TransformComponent>();
    goalFlagTr->SetTranslation({kGoalCenterX, kGoalCenterY + 0.95F, 0.03F});
    goalFlagTr->SetScale({1.35F, 2.1F, 1.0F});
    goalFlagGo->AddComponent<Spark::SpriteComponent>(
            goalFlagTex,
            Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
            Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
            18);
    goalFlagGo->AddComponent<Spark::SpriteLighting2DComponent>(
            SpriteLighting2DMode::PulseEmission,
            Spark::Vector4{1.0F, 0.92F, 0.45F, 1.0F},
            Spark::Vector4{1.35F, 0.42F, 0.0F, 0.0F});
    roots.Track(goalFlagGo);
}

void Platformer2DDemo::UpdateBackgroundParallax(const float cameraX) noexcept
{
    const float dx = cameraX - kBackgroundAnchorX;
    if (bgSkyTr != nullptr) {
        bgSkyTr->SetTranslation({kBackgroundAnchorX + dx * 0.04F, 4.2F, -0.45F});
    }
    if (bgMountainsTr != nullptr) {
        bgMountainsTr->SetTranslation({kBackgroundAnchorX + dx * 0.18F, 1.35F, -0.35F});
    }
    if (bgHillsTr != nullptr) {
        bgHillsTr->SetTranslation({kBackgroundAnchorX + dx * 0.32F, 0.55F, -0.25F});
    }
    if (bgCloudTrA != nullptr) {
        bgCloudTrA->SetTranslation({6.0F + dx * 0.12F + std::sin(sceneTime * 0.22F) * 0.35F, 6.8F, -0.15F});
    }
    if (bgCloudTrB != nullptr) {
        bgCloudTrB->SetTranslation({34.0F + dx * 0.10F + std::sin(sceneTime * 0.17F + 1.4F) * 0.45F, 7.4F, -0.14F});
    }
}

void Platformer2DDemo::UpdateGoalPresentation(const float deltaSeconds) noexcept
{
    goalPulse += deltaSeconds;
    const float pulse = 0.92F + 0.08F * std::sin(goalPulse * (goalReached ? 5.5F : 2.8F));
    if (goalGlowTr != nullptr) {
        const float glowScale = goalReached ? 3.2F : 2.4F;
        goalGlowTr->SetScale({glowScale * pulse, glowScale * pulse, 1.0F});
    }
    if (goalFlagTr != nullptr) {
        const float sway = std::sin(goalPulse * 3.4F) * 0.06F;
        goalFlagTr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitZ, sway));
        goalFlagTr->SetScale({1.35F * pulse, 2.1F * pulse, 1.0F});
    }
}

void Platformer2DDemo::RefreshStatusHud() noexcept
{
    if (goalReached) {
        std::snprintf(
                statusHudBuffer,
                sizeof(statusHudBuffer),
                "Gems %d/%d  |  Goal reached!  |  Enemies %d/%d",
                gemsCollected,
                gemsTotal,
                enemySquad.GetDefeatedCount(),
                Platformer2D::Config::kEnemyCount);
    } else {
        std::snprintf(
                statusHudBuffer,
                sizeof(statusHudBuffer),
                "Gems %d/%d  |  Reach the flag  |  Enemies %d/%d",
                gemsCollected,
                gemsTotal,
                enemySquad.GetDefeatedCount(),
                Platformer2D::Config::kEnemyCount);
    }
    helpHud.SetDetail(statusHudBuffer);
}

void Platformer2DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
    Unload(w);

    roots.Clear();
    for (std::size_t gi = 0; gi < gemObjects.GetSize(); ++gi) {
        if (gemObjects[gi] != nullptr) {
            w.DestroyGameObject(gemObjects[gi]);
        }
    }
    gemObjects.Clear();
    gemBasePositions.Clear();
    enemySquad.Unload(w);
    playerBullets.Shutdown(w);
    enemyBullets.Shutdown(w);
    explosions.Shutdown();
    healthHud.Shutdown(w);

    gemsCollected = 0;
    gemsTotal = 0;
    goalReached = false;
    wasGrounded = true;
    goalPulse = 0.0F;
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
        enemyIdleUv = SpriteAnimatorComponent::ComputeUniformGridUv(
                enemyAtlasColumns, 1U, 0U, enemyAtlasTex->GetWidth(), enemyAtlasTex->GetHeight());
        enemyAttackUv = enemyAtlasColumns >= 2U
                ? SpriteAnimatorComponent::ComputeUniformGridUv(
                          enemyAtlasColumns, 1U, 1U, enemyAtlasTex->GetWidth(), enemyAtlasTex->GetHeight())
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
    sfxPowerUp = LoadPlatformerSfx("assets/audio/power_up.wav");

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
            Spark::Vector4{1.02F, 0.98F, 0.94F, 1.0F},
            SpriteAnimatorComponent::ComputeUniformGridUv(
                    playerAtlasColumns,
                    kPlayerAtlasRows,
                    0,
                    playerAtlasTex->GetWidth(),
                    playerAtlasTex->GetHeight()),
            500);
    playerObject->AddComponent<Spark::SpriteLighting2DComponent>(
            SpriteLighting2DMode::PulseEmission,
            Spark::Vector4{0.55F, 0.78F, 1.0F, 0.35F},
            Spark::Vector4{1.2F, 0.38F, 0.0F, 0.0F});
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
    SpawnBackgroundLayers(w);

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
        gemBasePositions.PushBack(
                {kGemSpawns[static_cast<std::size_t>(gi)][0], kGemSpawns[static_cast<std::size_t>(gi)][1]});
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
    explosions.Initialize(w);

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
    cam->SetPriority(100);
    cameraRig = mainCameraGo->AddComponent<Spark::Camera2DRigComponent>();
    cameraRig->SetMode(Spark::Camera2DRigMode::BoundedFollow);
    cameraRig->SetTarget(playerObject);
    cameraRig->SetTargetOffset({0.0F, 1.48F, 0.0F});
    cameraRig->SetFollowSmoothRate(7.5F);
    cameraRig->SetUseBounds(true);
    cameraRig->SetBoundsMin({-8.0F, -1.5F});
    cameraRig->SetBoundsMax({50.0F, 9.0F});

    physics.GetQueries2D().RebuildStatics(w);

    helpHud.Mount(w, "2D platformer", DemoHelpHud::Style::DarkOnBright);
    helpHud.SetScreenOffset(Spark::DemoHud::kScreenMargin, 58.0F);
    helpHud.SetControlHints("WASD move | Space jump | J attack | TAB menu");
    RefreshStatusHud();

    context.GetInput().SetCursorCaptured(false);
}

void Platformer2DDemo::Unload(Spark::GameWorld& w)
{
    helpHud.Unmount(w);
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
    gemBasePositions.Clear();
    enemySquad.Unload(w);
    playerBullets.Shutdown(w);
    enemyBullets.Shutdown(w);
    explosions.Shutdown();
    healthHud.Shutdown(w);
    roots.DestroyAll(w);

    gemTex.Reset();
    platformTilesTex.Reset();
    playerAtlasTex.Reset();
    enemyAtlasTex.Reset();
    playerBulletTex.Reset();
    enemyBulletTex.Reset();
    hudWhiteTex.Reset();
    bgSkyTex.Reset();
    bgHillsTex.Reset();
    bgMountainsTex.Reset();
    bgCloudTex.Reset();
    goalFlagTex.Reset();
    sfxJump.Reset();
    sfxCoin.Reset();
    sfxExplosion.Reset();
    sfxHurt.Reset();
    sfxPowerUp.Reset();
    bgSkyGo = nullptr;
    bgSkyTr = nullptr;
    bgMountainsGo = nullptr;
    bgMountainsTr = nullptr;
    bgHillsGo = nullptr;
    bgHillsTr = nullptr;
    bgCloudGoA = nullptr;
    bgCloudTrA = nullptr;
    bgCloudGoB = nullptr;
    bgCloudTrB = nullptr;
    goalFlagGo = nullptr;
    goalFlagTr = nullptr;
    goalGlowGo = nullptr;
    goalGlowTr = nullptr;
    playerObject = nullptr;
    playerTr = nullptr;
    playerRb = nullptr;
    playerHealth = nullptr;
    playerDamageable = nullptr;
    playerAnim = nullptr;
    playerCharFsm = nullptr;
    mainCameraGo = nullptr;
    cameraRig = nullptr;

    physics = PhysicsSubsystem{};
    gemsCollected = 0;
    gemsTotal = 0;
    goalReached = false;
    wasGrounded = true;
    goalPulse = 0.0F;
    sceneTime = 0.0F;
    facingLeft = false;
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
    Spark::ProcessVfx(world);

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
        v.x = run * 11.0F;
        const bool jumpPressed = playerRb->IsGrounded() && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE);
        if (jumpPressed) {
            v.y = 13.2F;
            if (playerObject != nullptr && sfxJump.Get() != nullptr) {
                DemoAudio::QueueCue(*playerObject, sfxJump, 0.95F);
            }
        }
        playerRb->SetVelocity(v);
    }

    physics.Simulate2D(world, timing);

    if (playerRb != nullptr && playerTr != nullptr) {
        const bool attackPressed = in.IsKeyPressedThisFrame(GLFW_KEY_J);
        const Spark::Vector3 p = playerTr->GetLocalTransform().translation;
        const float healthBefore = playerHealth != nullptr ? playerHealth->GetCurrent() : 0.0F;
        const bool fired = playerCombat.TryFireOnAttackPressed(
                attackPressed,
                p.x,
                p.y,
                facingLeft,
                playerBullets,
                playerBulletProfile);
        if (fired) {
            const float dirX = facingLeft ? -1.0F : 1.0F;
            explosions.SpawnMuzzleFlash(
                    p.x + dirX * (kPlayerHalfW * 0.85F),
                    p.y + kPlayerHalfH * 0.12F);
            DemoPlayProceduralClip(context, DemoSfx::ClipPlatformerShoot(), 0.82F);
        }

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
        if (playerHealth != nullptr && playerHealth->GetCurrent() < healthBefore) {
            explosions.SpawnPlayerHurt(p.x, p.y);
        }

        const bool groundedNow = playerRb->IsGrounded();
        const Spark::Vector2 landingVel = playerRb->GetVelocity();
        if (groundedNow && !wasGrounded && landingVel.y < -2.5F) {
            explosions.SpawnLandDust(p.x, p.y - kPlayerHalfH + 0.05F);
            DemoPlayProceduralClip(context, DemoSfx::ClipPlatformerLand(), 0.55F);
        }
        wasGrounded = groundedNow;
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
            Spark::TransformComponent* gtr = gem->GetComponent<Spark::TransformComponent>();
            if (gtr == nullptr) {
                world.DestroyGameObject(gem);
                gemObjects.RemoveAt(gi);
                gemBasePositions.RemoveAt(gi);
                continue;
            }
            const float bob = std::sin(sceneTime * 4.2F + static_cast<float>(gi) * 0.73F) * 0.08F;
            const float spin = std::sin(sceneTime * 2.6F + static_cast<float>(gi) * 0.41F) * 0.04F;
            const float pulse = kGemDrawScale * (0.96F + 0.08F * std::sin(sceneTime * 5.5F + static_cast<float>(gi)));
            const Spark::Vector2 base = gemBasePositions[gi];
            gtr->SetTranslation({base.x, base.y + bob, 0.05F + 0.0003F * static_cast<float>(gi)});
            gtr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitZ, spin));
            gtr->SetScale({pulse, pulse, 1.0F});

            const Spark::Vector3 gpos = gtr->GetLocalTransform().translation;
            const float gdx = gpos.x - p.x;
            const float gdy = gpos.y - p.y;
            if (gdx * gdx + gdy * gdy <= gcr2) {
                explosions.SpawnGemPickup(gpos.x, gpos.y);
                world.DestroyGameObject(gem);
                gemObjects.RemoveAt(gi);
                gemBasePositions.RemoveAt(gi);
                ++gemsCollected;
                if (playerObject != nullptr && sfxCoin.Get() != nullptr) {
                    DemoAudio::QueueCue(*playerObject, sfxCoin, 0.95F);
                } else {
                    DemoPlayProceduralClip(context, DemoSfx::ClipGemCollect(), 0.88F);
                }
                continue;
            }
            ++gi;
        }

        if (!goalReached && std::fabs(p.x - kGoalCenterX) <= kGoalHalfW && std::fabs(p.y - kGoalCenterY) <= kGoalHalfH) {
            goalReached = true;
            explosions.SpawnGoalCelebration(kGoalCenterX, kGoalCenterY);
            if (playerObject != nullptr && sfxPowerUp.Get() != nullptr) {
                DemoAudio::QueueCue(*playerObject, sfxPowerUp, 1.0F);
            } else {
                DemoPlayProceduralClip(context, DemoSfx::ClipPlatformerGoal(), 0.95F);
            }
        }

        UpdateBackgroundParallax(p.x);
        UpdateGoalPresentation(dt);
        RefreshStatusHud();
    }

    if (mainCameraGo != nullptr) {
        if (Spark::Camera2DComponent* cam = mainCameraGo->GetComponent<Spark::Camera2DComponent>()) {
            int fbW = 0;
            int fbH = 0;
            context.GetFramebufferSize(fbW, fbH);
            healthHud.SyncToCamera(*cam, *mainCameraGo, static_cast<float>(fbW), static_cast<float>(fbH));
        }
    }

    if (playerHealth != nullptr) {
        healthHud.SetHealth(playerHealth->GetCurrent(), playerHealth->GetMaximum());
    }

    helpHud.Update(timing, context);
}

void Platformer2DDemo::Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context)
{
    (void)Spark::SubmitStandardLitSceneFromWorldWithCamera(
            world,
            context,
            Spark::Vector3{0.62F, 0.48F, 0.62F}.Normalized(),
            Spark::Vector3{1.0F, 0.94F, 0.88F},
            1.05F,
            Spark::Vector3{0.22F, 0.28F, 0.38F},
            true,
            sceneTime);

    Spark::SceneRenderParams* sceneParams = nullptr;
    if (context.TryGetMutableSceneRenderParams(sceneParams) && sceneParams != nullptr) {
        helpHud.PatchSceneRenderParams(*sceneParams, world);
    }
}

}  // namespace Spark
