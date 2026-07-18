#include "Platformer2DGame.hpp"

#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/scene/SceneSubmit.hpp"
#include "spark/config.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/IRenderFrame.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/text/Font.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace Spark {

namespace {

constexpr float kGroundTopY = -1.0F;
constexpr float kPlayerSpawnX = -5.0F;
constexpr float kFallRespawnY = -6.5F;
constexpr float kGoalMinX = 15.5F;
constexpr float kRunSpeed = 9.0F;
constexpr float kJumpSpeed = 11.5F;

void AddSolidPlatform(
        GameWorld& w,
        Array<GameObject*>& roots,
        const SharedPtr<Texture2D>& tex,
        float x0,
        float y0,
        float x1,
        float y1,
        const std::int32_t sortOrder,
        const Vector4& tint,
        const Vector4& uvRect) {
    const float cx = (x0 + x1) * 0.5F;
    const float cy = (y0 + y1) * 0.5F;
    const float sx = std::fabs(x1 - x0);
    const float sy = std::fabs(y1 - y0);
    GameObject* go = w.CreateGameObject();
    go->GetName() = Utf8String("Platform");
    TransformComponent* tr = go->AddComponent<TransformComponent>();
    tr->SetTranslation({cx, cy, 0.01F + 0.0004F * static_cast<float>(sortOrder)});
    tr->SetScale({sx, sy, 1.0F});
    go->AddComponent<SpriteComponent>(tex, tint, uvRect, sortOrder);
    go->AddComponent<BoxCollider2DComponent>();
    go->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
    roots.PushBack(go);
}

}  // namespace

void Platformer2DGame::MountUiFontIfNeeded(GameWorld& world) {
    if (world.GetUiFont()) {
        return;
    }
    auto uiFont = MakeShared<Font>();
    constexpr float kUiFontEmPx = 42.0F;
    bool fontOk = uiFont->TryLoadTrueTypeFromFile(SPARK_UI_FONT_PATH, kUiFontEmPx);
    if (!fontOk) {
        Utf8String buildTree(SPARK_BUILD_ASSETS_DIR);
        buildTree.AppendUtf8("/fonts/Roboto-Regular.ttf");
        fontOk = uiFont->TryLoadTrueTypeFromFile(buildTree.CStr(), kUiFontEmPx);
    }
    if (!fontOk) {
        Utf8String srcTree(SPARK_ASSETS_DIR);
        srcTree.AppendUtf8("/fonts/Roboto-Regular.ttf");
        fontOk = uiFont->TryLoadTrueTypeFromFile(srcTree.CStr(), kUiFontEmPx);
    }
    if (!fontOk) {
        fontOk = uiFont->TryLoadTrueTypeFromFile("assets/fonts/Roboto-Regular.ttf", kUiFontEmPx);
    }
    if (fontOk) {
        world.SetUiFont(uiFont);
        auto uiBold = MakeShared<Font>();
        bool boldOk = uiBold->TryLoadTrueTypeFromFile(SPARK_UI_BOLD_FONT_PATH, kUiFontEmPx);
        if (!boldOk) {
            Utf8String boldBuild(SPARK_BUILD_ASSETS_DIR);
            boldBuild.AppendUtf8("/fonts/Roboto-Bold.ttf");
            boldOk = uiBold->TryLoadTrueTypeFromFile(boldBuild.CStr(), kUiFontEmPx);
        }
        if (!boldOk) {
            Utf8String boldSrc(SPARK_ASSETS_DIR);
            boldSrc.AppendUtf8("/fonts/Roboto-Bold.ttf");
            boldOk = uiBold->TryLoadTrueTypeFromFile(boldSrc.CStr(), kUiFontEmPx);
        }
        if (boldOk) {
            world.SetUiBoldFont(uiBold);
        }
    }
}

void Platformer2DGame::OnAttach(IEngineContext& context) {
    GameWorld& world = GetWorld();
    MountUiFontIfNeeded(world);

    tileTex = MakeShared<Texture2D>(Utf8String("Plat2DTemplateTiles"));
    usingKenneyTiles = DemoAssets::TryLoadKenneySimplifiedPlatformerTilesheet(*tileTex);
    if (!usingKenneyTiles) {
        *tileTex = Texture2D::CreateCheckerboard(
                256,
                32,
                Vector3{0.38F, 0.34F, 0.30F},
                Vector3{0.16F, 0.48F, 0.30F});
        tileTex->GetName() = Utf8String("Plat2DTemplateChecker");
    }
    world.RegisterTexture(tileTex, "platformer2d_template/tiles");

    playerAtlasTex = MakeShared<Texture2D>(Utf8String("Plat2DTemplatePlayer"));
    usingKenneyPlayer = DemoAssets::TryBuildKenneyPlayerAtlas(*playerAtlasTex, playerAtlasColumns);
    if (!usingKenneyPlayer) {
        playerAtlasTex.Reset();
        playerAtlasColumns = 1U;
    } else {
        world.RegisterTexture(playerAtlasTex, "platformer2d_template/player");
    }

    static constexpr std::uint32_t kPlatformTileNumbers[] = {1U, 1U, 2U, 3U, 20U};
    static constexpr Vector4 kFullUv{0.0F, 0.0F, 1.0F, 1.0F};
    const auto platformUv = [this](std::size_t index) -> Vector4 {
        if (!usingKenneyTiles) {
            return kFullUv;
        }
        return DemoAssets::KenneySimplifiedPlatformerTileUv(kPlatformTileNumbers[index]);
    };

    AddSolidPlatform(
            world, roots, tileTex, -12.0F, -3.0F, 24.0F, kGroundTopY, 10, Vector4{0.92F, 0.90F, 0.86F, 1.0F},
            platformUv(0));
    AddSolidPlatform(
            world, roots, tileTex, -2.0F, -0.2F, 2.5F, 0.55F, 20, Vector4{0.88F, 0.86F, 0.82F, 1.0F},
            platformUv(1));
    AddSolidPlatform(
            world, roots, tileTex, 4.5F, 1.0F, 9.0F, 1.65F, 21, Vector4{0.84F, 0.82F, 0.78F, 1.0F},
            platformUv(2));
    AddSolidPlatform(
            world, roots, tileTex, 11.0F, 2.4F, 16.5F, 3.05F, 22, Vector4{0.80F, 0.78F, 0.74F, 1.0F},
            platformUv(3));
    AddSolidPlatform(
            world, roots, tileTex, 17.0F, 0.15F, 22.5F, 0.95F, 19, Vector4{0.72F, 0.92F, 0.68F, 1.0F},
            platformUv(4));

    const SharedPtr<Texture2D>& playerTex = usingKenneyPlayer ? playerAtlasTex : tileTex;
    const Vector4 playerUv =
            usingKenneyPlayer
                    ? SpriteAnimatorComponent::ComputeUniformGridUv(playerAtlasColumns, 1U, 0U)
                    : kFullUv;
    const Vector4 playerTint =
            usingKenneyPlayer ? Vector4{0.98F, 0.96F, 0.94F, 1.0F} : Vector4{0.35F, 0.55F, 0.95F, 1.0F};

    playerObject = world.CreateGameObject();
    playerObject->GetName() = Utf8String("Player");
    playerTr = playerObject->AddComponent<TransformComponent>();
    playerTr->SetScale({playerBaseScaleX, playerBaseScaleY, 1.0F});
    const float spawnY = kGroundTopY + 0.5F * playerBaseScaleY;
    playerTr->SetTranslation({kPlayerSpawnX, spawnY, 0.04F});
    playerObject->AddComponent<SpriteComponent>(playerTex, playerTint, playerUv, 500);
    playerObject->AddComponent<BoxCollider2DComponent>();
    playerRb = playerObject->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Dynamic, 1.0F);
    roots.PushBack(playerObject);

    GameObject* hud = world.CreateGameObject();
    hud->GetName() = Utf8String("Hud");
    hudText = hud->AddComponent<TextOverlayComponent>();
    hudText->SetScreenPosition(12.0F, 12.0F);
    hudText->SetFontSizePixels(20.0F);
    hudText->SetColor({0.94F, 0.97F, 1.0F});
    hudText->SetText(Utf8String("2D platformer — A/D move · Space jump · reach right -> goal"));
    roots.PushBack(hud);

    camera.position = {kPlayerSpawnX, spawnY + 1.0F, 0.0F};
    camera.halfExtentY = 6.5F;
    camera.rotationRad = 0.0F;

    context.GetInput().SetCursorCaptured(false);
    glfwSetWindowTitle(context.GetWindow().Handle(), "Spark 2D platformer template");
}

void Platformer2DGame::OnDetach() {
    GameWorld& world = GetWorld();
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    playerObject = nullptr;
    playerTr = nullptr;
    playerRb = nullptr;
    hudText = nullptr;
    tileTex.Reset();
}

void Platformer2DGame::CloseWindowIfRequested(IEngineContext& context) const {
    if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(context.GetWindow().Handle(), GLFW_TRUE);
    }
}

void Platformer2DGame::OnUpdate(const FrameTiming& timing, IEngineContext& context) {
    sceneTimeSeconds = timing.totalTimeSeconds;
    CloseWindowIfRequested(context);

    if (playerRb != nullptr && playerTr != nullptr) {
        IInput& in = context.GetInput();
        float run = 0.0F;
        if (in.IsKeyDown(GLFW_KEY_A) || in.IsKeyDown(GLFW_KEY_LEFT)) {
            run -= 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_D) || in.IsKeyDown(GLFW_KEY_RIGHT)) {
            run += 1.0F;
        }
        if (std::fabs(run) > 0.5F) {
            facingLeft = (run < 0.0F);
        }
        playerTr->SetScale({facingLeft ? -playerBaseScaleX : playerBaseScaleX, playerBaseScaleY, 1.0F});

        Vector2 v = playerRb->GetVelocity();
        v.x = run * kRunSpeed;
        if (playerRb->IsGrounded() && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
            v.y = kJumpSpeed;
        }
        playerRb->SetVelocity(v);

        PhysicsWorld2DSettings phys{};
        phys.gravityY = -30.0F;
        phys.maxFallSpeed = 42.0F;
        SimulatePhysics2D(GetWorld(), timing, phys);

        const Vector3 p = playerTr->GetLocalTransform().translation;
        if (p.y < kFallRespawnY) {
            const float spawnY = kGroundTopY + 0.5F * playerBaseScaleY;
            playerTr->SetTranslation({kPlayerSpawnX, spawnY, p.z});
            playerRb->SetVelocity(Vector2::Zero);
            goalReached = false;
        }
        if (!goalReached && p.x >= kGoalMinX) {
            goalReached = true;
        }

        const float follow = std::min(1.0F, 8.0F * timing.deltaTimeSeconds);
        camera.position.x += (p.x - camera.position.x) * follow;
        camera.position.y += ((p.y + 0.85F) - camera.position.y) * follow;
    }

    if (hudText != nullptr) {
        const std::string msg = std::format(
                "2D platformer {} | pos ({:.1f},{:.1f}) | A/D Space | ESC quit",
                goalReached ? "— GOAL!" : "",
                static_cast<double>(playerTr ? playerTr->GetLocalTransform().translation.x : 0.0),
                static_cast<double>(playerTr ? playerTr->GetLocalTransform().translation.y : 0.0));
        hudText->SetText(Utf8String(msg.c_str()));
    }

    Game::OnUpdate(timing, context);
}

void Platformer2DGame::OnRender(IRenderFrame& /*frame*/, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }

    const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
    Vector3 pr{};
    Vector3 pu{};
    camera.BillboardBasisWorld(pr, pu);

    SubmitStandardLitSceneFromWorld(
            GetWorld(),
            context,
            viewProj,
            camera.position,
            Vector3{0.30F, 0.86F, 0.36F}.Normalized(),
            Vector3{1.0F, 0.98F, 0.95F},
            0.85F,
            Vector3{0.16F, 0.18F, 0.24F},
            false,
            pr,
            pu,
            sceneTimeSeconds,
            SceneSpriteSortMode::SortOrderThenWorldY);
}

}  // namespace Spark
