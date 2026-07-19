#include "spark/demo/SpaceInvaders2DDemo.hpp"

#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/audio/SoundFileLoader.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

namespace Spark {

void SpaceInvaders2DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        usingBundledShipArt = false;
        usingBundledProjectileArt = false;

        shipsTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("SpaceInvadersShips"));
        if (DemoAssets::TryLoadSpaceShooterShips(*shipsTex)) {
            usingBundledShipArt = true;
        } else {
            shipsTex = Detail::MakeSpaceInvadersAtlas();
        }
        w.RegisterTexture(shipsTex, "spark/spaceinv/ships");

        projectilesTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("SpaceInvadersProjectiles"));
        if (DemoAssets::TryLoadSpaceShooterProjectiles(*projectilesTex)) {
            usingBundledProjectileArt = true;
        } else {
            projectilesTex = shipsTex;
        }
        w.RegisterTexture(projectilesTex, "spark/spaceinv/projectiles");

        fxTex = Detail::MakeSpaceInvadersAtlas();
        w.RegisterTexture(fxTex, "spark/spaceinv/fx");

        hudGo = w.CreateGameObject();
        hudGo->GetName() = Spark::Utf8String("SpaceInvadersHud");
        hudText = hudGo->AddComponent<Spark::TextOverlayComponent>();
        hudText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*hudText, false);
        roots.PushBack(hudGo);

        aliens.Clear();
        aliens.Resize(static_cast<std::size_t>(kAlienCols * kAlienRows));
        for (int j = 0; j < kAlienRows; ++j) {
            for (int i = 0; i < kAlienCols; ++i) {
                const int idx = j * kAlienCols + i;
                Spark::GameObject* go = w.CreateGameObject();
                go->GetName() = Spark::Utf8String("Alien");
                Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
                tr->SetUniformScale(usingBundledShipArt ? Detail::kBundledAlienScale : 0.88F);
                const std::uint32_t shipFrame = Detail::AlienShipFrame(j, i, false);
                Spark::SpriteComponent* spr = go->AddComponent<Spark::SpriteComponent>(
                        shipsTex,
                        Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                        usingBundledShipArt ? Detail::ShipUv(shipFrame) : Detail::LegacyAtlasUv(shipFrame % 2U),
                        40 + idx);
                roots.PushBack(go);
                aliens[static_cast<std::size_t>(idx)].go = go;
                aliens[static_cast<std::size_t>(idx)].tr = tr;
                aliens[static_cast<std::size_t>(idx)].spr = spr;
                aliens[static_cast<std::size_t>(idx)].alive = true;
                aliens[static_cast<std::size_t>(idx)].gi = i;
                aliens[static_cast<std::size_t>(idx)].gj = j;
            }
        }

        playerGo = w.CreateGameObject();
        playerGo->GetName() = Spark::Utf8String("Player");
        playerTr = playerGo->AddComponent<Spark::TransformComponent>();
        playerTr->SetUniformScale(usingBundledShipArt ? Detail::kBundledPlayerScale : 1.05F);
        playerTr->SetTranslation({playerX, playerY, 0.08F});
        playerSpr = playerGo->AddComponent<Spark::SpriteComponent>(
                shipsTex,
                Spark::Vector4{0.55F, 0.98F, 1.0F, 1.0F},
                usingBundledShipArt ? Detail::ShipUv(Detail::kPlayerShipFrame) : Detail::LegacyAtlasUv(2U),
                900);
        roots.PushBack(playerGo);

        if (!usingBundledShipArt) {
            playerShadowGo = w.CreateGameObject();
            playerShadowGo->GetName() = Spark::Utf8String("PlayerShadow");
            {
                Spark::TransformComponent* tr = playerShadowGo->AddComponent<Spark::TransformComponent>();
                tr->SetUniformScale(1.18F);
                tr->SetTranslation({playerX, playerY - 0.22F, 0.02F});
            }
            playerShadowGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Multiply);
            playerShadowGo->AddComponent<Spark::SpriteComponent>(
                    shipsTex,
                    Spark::Vector4{0.08F, 0.10F, 0.16F, 0.72F},
                    Detail::LegacyAtlasUv(2U),
                    499);
            roots.PushBack(playerShadowGo);
        } else {
            playerShadowGo = nullptr;
        }

        playerBullets.Clear();
        playerBullets.Resize(static_cast<std::size_t>(kMaxPlayerBullets));
        for (int b = 0; b < kMaxPlayerBullets; ++b) {
            Spark::GameObject* go = w.CreateGameObject();
            go->GetName() = Spark::Utf8String("PBullet");
            Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
            tr->SetUniformScale(usingBundledProjectileArt ? Detail::kBundledPlayerBulletScale : 0.22F);
            go->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
            Spark::SpriteComponent* spr =
                    go->AddComponent<Spark::SpriteComponent>(
                            projectilesTex,
                            Spark::Vector4{1.0F, 1.0F, 1.0F, 0.0F},
                            usingBundledProjectileArt ? Detail::ProjectileUv(Detail::kPlayerBulletFrame)
                                                      : Detail::LegacyAtlasUv(3U),
                            450);
            roots.PushBack(go);
            playerBullets[static_cast<std::size_t>(b)] = {false, go, tr, spr, 0.0F, 0.0F};
        }

        enemyBullets.Clear();
        enemyBullets.Resize(static_cast<std::size_t>(kMaxEnemyBullets));
        for (int b = 0; b < kMaxEnemyBullets; ++b) {
            Spark::GameObject* go = w.CreateGameObject();
            go->GetName() = Spark::Utf8String("EBullet");
            Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
            tr->SetUniformScale(usingBundledProjectileArt ? Detail::kBundledEnemyBulletScale : 0.2F);
            Spark::SpriteComponent* spr = go->AddComponent<Spark::SpriteComponent>(
                    projectilesTex,
                    Spark::Vector4{1.0F, 0.45F, 0.12F, 0.0F},
                    usingBundledProjectileArt ? Detail::ProjectileUv(Detail::kEnemyBulletFrame)
                                              : Detail::LegacyAtlasUv(3U),
                    448);
            roots.PushBack(go);
            enemyBullets[static_cast<std::size_t>(b)] = {false, go, tr, spr, 0.0F, 0.0F};
        }

        explosions.Clear();
        explosions.Resize(12);
        for (std::size_t e = 0; e < explosions.GetSize(); ++e) {
            Spark::GameObject* go = w.CreateGameObject();
            go->GetName() = Spark::Utf8String("Explosion");
            Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
            tr->SetUniformScale(1.4F);
            tr->SetTranslation({-120.0F, -120.0F, 0.05F});
            go->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
            Spark::SpriteComponent* spr = go->AddComponent<Spark::SpriteComponent>(
                    fxTex,
                    Spark::Vector4{1.0F, 0.72F, 0.28F, 0.0F},
                    Detail::LegacyAtlasUv(1U),
                    960 + static_cast<int>(e));
            roots.PushBack(go);
            explosions[e] = {false, go, tr, spr, 0.0F};
        }

        camera.position = {kWorldW * 0.5F, kWorldH * 0.5F, 0.0F};
        camera.halfExtentY = kWorldH * 0.52F;
        camera.rotationRad = 0.0F;

        RandSeed(timingHackU32(context));
        ResetRound();
        context.GetInput().SetCursorCaptured(false);

        if (audioEngine != nullptr) {
            audioEngine->ClearBackgroundMusic();
            audioEngine = nullptr;
        }
        audioEngine = context.TryGetSoundEngine();
        if (audioEngine != nullptr && audioEngine->IsRunning()) {
            if (Spark::SharedPtr<Spark::SoundClip> bgm =
                        TryLoadSoundClipFromBundledAsset("assets/audio/SpaceRangers.wav")) {
                audioEngine->SetBackgroundMusic(bgm, 0.30F, true);
            }
        }
    }

void SpaceInvaders2DDemo::Unload(Spark::GameWorld& w)
{
        if (audioEngine != nullptr) {
            audioEngine->ClearBackgroundMusic();
            audioEngine = nullptr;
        }
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        shipsTex.Reset();
        projectilesTex.Reset();
        fxTex.Reset();
        usingBundledShipArt = false;
        usingBundledProjectileArt = false;
        hudGo = nullptr;
        hudText = nullptr;
        playerGo = nullptr;
        playerTr = nullptr;
        playerSpr = nullptr;
        playerShadowGo = nullptr;
        aliens.Clear();
        playerBullets.Clear();
        enemyBullets.Clear();
        explosions.Clear();
    }

void SpaceInvaders2DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        Spark::IInput& in = context.GetInput();
        const float dt = timing.deltaTimeSeconds;
        TickExplosions(dt);

        if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
            ResetRound();
        }

        if (gamePhase == 0) {
            const float move = 10.0F * dt;
            if (in.IsKeyDown(GLFW_KEY_LEFT) || in.IsKeyDown(GLFW_KEY_A)) {
                playerX = std::max(1.2F, playerX - move);
            }
            if (in.IsKeyDown(GLFW_KEY_RIGHT) || in.IsKeyDown(GLFW_KEY_D)) {
                playerX = std::min(kWorldW - 1.2F, playerX + move);
            }
            fireCooldown = std::max(0.0F, fireCooldown - dt);
            if ((in.IsKeyDown(GLFW_KEY_SPACE) || in.IsKeyDown(GLFW_KEY_UP)) && fireCooldown <= 0.0F) {
                if (TrySpawnPlayerBullet()) {
                    fireCooldown = 0.32F;
                    DemoPlayProceduralClip(context, DemoSfx::ClipInvadersShoot(), 1.0F);
                }
            }

            fleetX += fleetVelX * dt;
            float minAx = 1.0e9F;
            float maxAx = -1.0e9F;
            for (std::size_t i = 0; i < aliens.GetSize(); ++i) {
                if (!aliens[i].alive) {
                    continue;
                }
                const float ax = fleetX + static_cast<float>(aliens[i].gi) * kStepX;
                minAx = std::min(minAx, ax);
                maxAx = std::max(maxAx, ax);
            }
            if (minAx < 1.1F || maxAx > kWorldW - 1.1F) {
                fleetVelX = -fleetVelX;
                fleetX += fleetVelX * dt * 1.5F;
                fleetY -= 0.45F;
            }

            enemyFireTimer -= dt;
            if (enemyFireTimer <= 0.0F) {
                enemyFireTimer = 0.55F + static_cast<float>(RandU32() % 70U) * 0.01F;
                TrySpawnEnemyBullet();
            }

            SyncAlienTransforms((timing.frameIndex / 12U) % 2U == 0U);
            playerTr->SetTranslation({playerX, playerY, 0.08F});
            UpdatePlayerShadow();

            for (std::size_t b = 0; b < playerBullets.GetSize(); ++b) {
                BulletSlot& bl = playerBullets[b];
                if (!bl.active) {
                    continue;
                }
                bl.cx += bl.vx * dt;
                bl.cy += bl.vy * dt;
                bl.tr->SetTranslation({bl.cx, bl.cy, 0.035F});
                if (bl.cy > kWorldH - 0.25F) {
                    DeactivateBullet(bl);
                }
            }
            for (std::size_t b = 0; b < enemyBullets.GetSize(); ++b) {
                BulletSlot& bl = enemyBullets[b];
                if (!bl.active) {
                    continue;
                }
                bl.cx += bl.vx * dt;
                bl.cy += bl.vy * dt;
                bl.tr->SetTranslation({bl.cx, bl.cy, 0.036F});
                if (bl.cy < 0.25F) {
                    DeactivateBullet(bl);
                }
            }

            ResolveCollisions(context);

            int alive = 0;
            for (std::size_t i = 0; i < aliens.GetSize(); ++i) {
                if (aliens[i].alive) {
                    ++alive;
                    const float ay = AlienWorldY(fleetY, aliens[i].gj);
                    if (ay <= kLoseLineY) {
                        gamePhase = 2;
                    }
                }
            }
            if (alive == 0) {
                gamePhase = 1;
            }
        }

        if (hudText != nullptr) {
            const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            const char* phase = (gamePhase == 0) ? "PLAY" : (gamePhase == 1) ? "YOU WIN" : "GAME OVER";
            const char* art = usingBundledShipArt ? "SpaceShooter art" : "fallback art";
            hudText->SetText(Spark::Utf8String(
                    std::format(
                            "Space Invaders — {:.0f} FPS — {} — score {} · lives {} — {} — R restart · ESC menu",
                            static_cast<double>(fpsSmoothed),
                            phase,
                            score,
                            lives,
                            art)
                            .c_str()));
        }
    }

void SpaceInvaders2DDemo::Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        if (fbW <= 0) {
            fbW = 1;
        }
        if (fbH <= 0) {
            fbH = 1;
        }
        const Spark::Matrix4 viewProj =
                camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
        Spark::Vector3 pr{};
        Spark::Vector3 pu{};
        camera.BillboardBasisWorld(pr, pu);
        Spark::SubmitStandardLitSceneFromWorld(
                world,
                context,
                viewProj,
                camera.position,
                Spark::Vector3{0.28F, 0.55F, 0.92F}.Normalized(),
                Spark::Vector3{0.9F, 0.95F, 1.0F},
                0.55F,
                Spark::Vector3{0.04F, 0.05F, 0.09F},
                false,
                pr,
                pu,
                0.0F);
    }

std::uint32_t SpaceInvaders2DDemo::timingHackU32(Spark::IEngineContext& context)
{
        int ww = 0;
        int hh = 0;
        context.GetFramebufferSize(ww, hh);
        return static_cast<std::uint32_t>(ww * 7919 + hh * 65537 + 19);
    }

void SpaceInvaders2DDemo::RandSeed(std::uint32_t s) noexcept
{ rng = s != 0 ? s : 1U; }

[[nodiscard]] std::uint32_t SpaceInvaders2DDemo::RandU32() noexcept
{
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }

void SpaceInvaders2DDemo::ResetRound() noexcept
{
        score = 0;
        lives = 3;
        gamePhase = 0;
        fleetX = 3.5F;
        fleetY = 19.0F;
        fleetVelX = 4.2F;
        playerX = kWorldW * 0.5F;
        playerY = 2.45F;
        fireCooldown = 0.0F;
        enemyFireTimer = 0.4F;
        for (std::size_t i = 0; i < aliens.GetSize(); ++i) {
            aliens[i].alive = true;
        }
        for (std::size_t b = 0; b < playerBullets.GetSize(); ++b) {
            DeactivateBullet(playerBullets[b]);
        }
        for (std::size_t b = 0; b < enemyBullets.GetSize(); ++b) {
            DeactivateBullet(enemyBullets[b]);
        }
        SyncAlienTransforms(true);
        if (playerTr != nullptr) {
            playerTr->SetTranslation({playerX, playerY, 0.08F});
        }
        UpdatePlayerShadow();
        for (std::size_t e = 0; e < explosions.GetSize(); ++e) {
            ExplosionSlot& ex = explosions[e];
            ex.active = false;
            ex.timeLeft = 0.0F;
            if (ex.tr != nullptr) {
                ex.tr->SetTranslation({-120.0F, -120.0F, 0.05F});
            }
            if (ex.spr != nullptr) {
                const Spark::Vector4 t = ex.spr->GetTint();
                ex.spr->SetTint({t.x, t.y, t.z, 0.0F});
            }
        }
    }

void SpaceInvaders2DDemo::SyncAlienTransforms(const bool animPhase) noexcept
{
        for (std::size_t i = 0; i < aliens.GetSize(); ++i) {
            AlienSlot& a = aliens[i];
            if (!a.alive) {
                a.tr->SetTranslation({-100.0F, -100.0F, 0.0F});
                continue;
            }
            const float ax = fleetX + static_cast<float>(a.gi) * kStepX;
            const float ay = AlienWorldY(fleetY, a.gj);
            a.tr->SetTranslation({ax, ay, 0.03F});
            if (a.spr != nullptr && usingBundledShipArt) {
                a.spr->SetUvRect(Detail::ShipUv(Detail::AlienShipFrame(a.gj, a.gi, animPhase)));
            }
        }
    }

void SpaceInvaders2DDemo::DeactivateBullet(BulletSlot& b) noexcept
{
        b.active = false;
        b.tr->SetTranslation({-80.0F, -80.0F, 0.0F});
        if (b.spr != nullptr) {
            const Spark::Vector4 t = b.spr->GetTint();
            b.spr->SetTint({t.x, t.y, t.z, 0.0F});
        }
    }

[[nodiscard]] bool SpaceInvaders2DDemo::TrySpawnPlayerBullet() noexcept
{
        for (std::size_t b = 0; b < playerBullets.GetSize(); ++b) {
            BulletSlot& bl = playerBullets[b];
            if (bl.active) {
                continue;
            }
            bl.active = true;
            bl.vx = 0.0F;
            bl.vy = 24.0F;
            bl.cx = playerX;
            bl.cy = playerY + 0.6F;
            bl.tr->SetTranslation({bl.cx, bl.cy, 0.035F});
            if (bl.spr != nullptr) {
                bl.spr->SetTint({1.0F, 1.0F, 1.0F, 1.0F});
            }
            return true;
        }
        return false;
    }

void SpaceInvaders2DDemo::TrySpawnEnemyBullet() noexcept
{
        const int col = static_cast<int>(RandU32() % static_cast<std::uint32_t>(kAlienCols));
        int pick = -1;
        for (int row = kAlienRows - 1; row >= 0; --row) {
            const int idx = row * kAlienCols + col;
            if (aliens[static_cast<std::size_t>(idx)].alive) {
                pick = idx;
                break;
            }
        }
        if (pick < 0) {
            return;
        }
        for (std::size_t b = 0; b < enemyBullets.GetSize(); ++b) {
            BulletSlot& bl = enemyBullets[b];
            if (bl.active) {
                continue;
            }
            const AlienSlot& a = aliens[static_cast<std::size_t>(pick)];
            const float ax = fleetX + static_cast<float>(a.gi) * kStepX;
            const float ay = AlienWorldY(fleetY, a.gj);
            bl.active = true;
            bl.vx = 0.0F;
            bl.vy = -11.0F;
            bl.cx = ax;
            bl.cy = ay - 0.55F;
            bl.tr->SetTranslation({bl.cx, bl.cy, 0.036F});
            if (bl.spr != nullptr) {
                bl.spr->SetTint({1.0F, 0.45F, 0.12F, 1.0F});
            }
            return;
        }
    }

bool SpaceInvaders2DDemo::BoxOverlap(float ax, float ay, float ahx, float ahy, float bx, float by, float bhx, float bhy) noexcept
{
        return std::fabs(ax - bx) <= ahx + bhx && std::fabs(ay - by) <= ahy + bhy;
    }

[[nodiscard]] float SpaceInvaders2DDemo::AlienWorldY(float fleetYVal, int gj) noexcept
{
        return fleetYVal - static_cast<float>(gj) * kStepY;
    }

void SpaceInvaders2DDemo::ResolveCollisions(Spark::IEngineContext& context) noexcept
{
        constexpr float ahx = 0.48F;
        constexpr float ahy = 0.34F;
        constexpr float phx = 0.55F;
        constexpr float phy = 0.32F;
        constexpr float bhx = 0.1F;
        constexpr float bhy = 0.22F;

        for (std::size_t bi = 0; bi < playerBullets.GetSize(); ++bi) {
            BulletSlot& pb = playerBullets[bi];
            if (!pb.active) {
                continue;
            }
            const float px = pb.cx;
            const float py = pb.cy;
            for (std::size_t ai = 0; ai < aliens.GetSize(); ++ai) {
                AlienSlot& al = aliens[ai];
                if (!al.alive) {
                    continue;
                }
                const float ax = fleetX + static_cast<float>(al.gi) * kStepX;
                const float ay = AlienWorldY(fleetY, al.gj);
                if (BoxOverlap(px, py, bhx, bhy, ax, ay, ahx, ahy)) {
                    al.alive = false;
                    DeactivateBullet(pb);
                    SpawnExplosion(ax, ay);
                    score += 10;
                    DemoPlayProceduralClip(context, DemoSfx::ClipInvadersHit(), 0.95F);
                    break;
                }
            }
        }

        for (std::size_t bi = 0; bi < enemyBullets.GetSize(); ++bi) {
            BulletSlot& eb = enemyBullets[bi];
            if (!eb.active || gamePhase != 0) {
                continue;
            }
            if (BoxOverlap(eb.cx, eb.cy, bhx, bhy, playerX, playerY, phx, phy)) {
                DeactivateBullet(eb);
                --lives;
                if (lives <= 0) {
                    gamePhase = 2;
                }
            }
        }
    }

void SpaceInvaders2DDemo::UpdatePlayerShadow() noexcept
{
        if (playerShadowGo == nullptr) {
            return;
        }
        if (Spark::TransformComponent* tr = playerShadowGo->GetComponent<Spark::TransformComponent>()) {
            tr->SetTranslation({playerX, playerY - 0.22F, 0.02F});
        }
    }

void SpaceInvaders2DDemo::SpawnExplosion(float worldX, float worldY) noexcept
{
        for (std::size_t e = 0; e < explosions.GetSize(); ++e) {
            ExplosionSlot& ex = explosions[e];
            if (ex.active) {
                continue;
            }
            ex.active = true;
            ex.timeLeft = 0.42F;
            if (ex.tr != nullptr) {
                ex.tr->SetTranslation({worldX, worldY, 0.05F});
                ex.tr->SetUniformScale(1.4F);
            }
            if (ex.spr != nullptr) {
                ex.spr->SetTint({1.0F, 0.72F, 0.28F, 0.95F});
            }
            return;
        }
    }

void SpaceInvaders2DDemo::TickExplosions(float dt) noexcept
{
        for (std::size_t e = 0; e < explosions.GetSize(); ++e) {
            ExplosionSlot& ex = explosions[e];
            if (!ex.active) {
                continue;
            }
            ex.timeLeft -= dt;
            const float pulse = std::max(0.0F, ex.timeLeft / 0.42F);
            if (ex.tr != nullptr) {
                ex.tr->SetUniformScale(1.4F + (1.0F - pulse) * 1.6F);
            }
            if (ex.spr != nullptr) {
                ex.spr->SetTint({1.0F, 0.72F, 0.28F, 0.95F * pulse * pulse});
            }
            if (ex.timeLeft <= 0.0F) {
                ex.active = false;
                if (ex.tr != nullptr) {
                    ex.tr->SetTranslation({-120.0F, -120.0F, 0.05F});
                }
                if (ex.spr != nullptr) {
                    ex.spr->SetTint({1.0F, 0.72F, 0.28F, 0.0F});
                }
            }
        }
    }
}  // namespace Spark
