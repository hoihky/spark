#include "spark/demo/Platformer2DDemo.hpp"
#include "spark/demo/Platformer2DDemo_detail.hpp"
#include "spark/ecs/components/Camera2DComponent.hpp"
#include "spark/ecs/components/Camera2DRigComponent.hpp"
#include "spark/scene/SceneSubmit.hpp"
#include "spark/ecs/components/Sprite2DCharacterAnimFsmComponent.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"

namespace Spark {

namespace {

constexpr float kAttackArcRadius = 1.5F;
constexpr float kAttackArcHalfAngleRad = 0.96F;

}  // namespace

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

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("PlatFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(12.0F, 12.0F);
        fpsText->SetFontSizePixels(20.0F);
        fpsText->SetColor({0.95F, 0.98F, 0.96F});
        {
            const char* plat = platformUsingKenneyTilesheet ? "Kenney tilesheet" : "checker fallback";
            const char* src = playerUsingKenneyAtlas
                    ? (playerAtlasColumns >= 5U
                               ? "Kenney character (idle, walk, happy=attack, duck=hurt)"
                               : "Kenney character (idle+walk only; add happy+duck PNGs for J/K)")
                    : "procedural 5-cell atlas (orange=attack, blue=hurt)";
            fpsText->SetText(Spark::Utf8String(
                    std::format(
                            "Platformer — {} — {} — WASD · Space jump · J attack (arc) · K hurt · fall = respawn",
                            plat,
                            src)
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
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        gemTex.Reset();
        platformTilesTex.Reset();
        playerAtlasTex.Reset();
        playerObject = nullptr;
        playerTr = nullptr;
        playerRb = nullptr;
        playerAnim = nullptr;
        playerCharFsm = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
        mainCameraGo = nullptr;
        cameraRig = nullptr;
        attackArcHitsScratch.Clear();
    }

void Platformer2DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world)
{
        sceneTime += timing.deltaTimeSeconds;
        Spark::IInput& in = context.GetInput();
        const float dt = timing.deltaTimeSeconds;

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
                weaponFilter.queryMaskBits = kGemHurtboxCategoryBits;
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
                    for (std::size_t gi = 0; gi < gemObjects.GetSize(); ++gi) {
                        if (gemObjects[gi] != hitOwner) {
                            continue;
                        }
                        world.DestroyGameObject(hitOwner);
                        gemObjects.RemoveAt(gi);
                        ++gemsCollected;
                        DemoPlayProceduralClip(context, DemoSfx::ClipGemCollect(), 0.95F);
                        break;
                    }
                }
            }
            if (p.y < kFallRespawnY) {
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
                    DemoPlayProceduralClip(context, DemoSfx::ClipGemCollect(), 0.95F);
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
                    "Platformer — {:.0f} FPS — gems {}/{} — WASD · Space jump · J attack (arc) · K hurt · fall = respawn",
                    static_cast<double>(fpsSmoothed),
                    gemsCollected,
                    gemsTotal);
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
