#include "spark/demo/TwoDDemo.hpp"
#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/ecs/components/BlendModeComponent.hpp"

namespace Spark {

void TwoDDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        checkerTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("TwoDTilesheet"));
        const bool usingKenneyAtlas = DemoAssets::TryLoadKenneyTinyDungeonAtlas(*checkerTex);
        if (!usingKenneyAtlas) {
            *checkerTex = Spark::Texture2D::CreateCheckerboard(
                    256,
                    32,
                    Spark::Vector3{0.88F, 0.92F, 0.96F},
                    Spark::Vector3{0.20F, 0.36F, 0.52F});
            checkerTex->GetName() = Spark::Utf8String("TwoDCheckerFallback");
        }
        w.RegisterTexture(checkerTex, "spark/2d/checker");

        const std::uint32_t kAtlasCols = usingKenneyAtlas ? 12U : 4U;
        const std::uint32_t kAtlasRows = usingKenneyAtlas ? 11U : 2U;
        mapObject = w.CreateGameObject();
        mapObject->GetName() = Spark::Utf8String("Tilemap");
        mapObject->AddComponent<Spark::TransformComponent>();
        constexpr std::uint32_t kMapW = 10;
        constexpr std::uint32_t kMapH = 8;
        Spark::TilemapComponent* tm = mapObject->AddComponent<Spark::TilemapComponent>(
                checkerTex, kMapW, kMapH, kAtlasCols, kAtlasRows, 1.0F, 0);
        for (std::uint32_t y = 0; y < kMapH; ++y) {
            for (std::uint32_t x = 0; x < kMapW; ++x) {
                const std::uint16_t id = usingKenneyAtlas
                        ? static_cast<std::uint16_t>((x + y * 3U) % (kAtlasCols * kAtlasRows))
                        : static_cast<std::uint16_t>(((x / 2) + (y / 2)) % 4);
                tm->SetTile(x, y, id);
            }
        }
        roots.PushBack(mapObject);

        spriteRed = w.CreateGameObject();
        spriteRed->GetName() = Spark::Utf8String("SpriteA");
        spriteRedTr = spriteRed->AddComponent<Spark::TransformComponent>();
        spriteRedTr->SetTranslation({4.5F, 3.5F, 0.02F});
        spriteRedTr->SetUniformScale(1.25F);
        spriteRed->AddComponent<Spark::SpriteComponent>(
                checkerTex,
                Spark::Vector4{1.0F, 0.38F, 0.30F, 0.92F},
                Spark::Vector4{0.0F, 0.0F, 0.5F, 0.5F},
                300);
        roots.PushBack(spriteRed);

        spriteGreen = w.CreateGameObject();
        spriteGreen->GetName() = Spark::Utf8String("SpriteB");
        {
            Spark::TransformComponent* tr = spriteGreen->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({5.2F, 3.85F, 0.03F});
            tr->SetUniformScale(1.0F);
        }
        spriteGreen->AddComponent<Spark::SpriteComponent>(
                checkerTex,
                Spark::Vector4{0.22F, 0.95F, 0.48F, 0.88F},
                Spark::Vector4{0.5F, 0.5F, 1.0F, 1.0F},
                400);
        roots.PushBack(spriteGreen);

        Spark::GameObject* waterTint = w.CreateGameObject();
        waterTint->GetName() = Spark::Utf8String("WaterTint");
        {
            Spark::TransformComponent* tr = waterTint->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({7.2F, 2.1F, 0.01F});
            tr->SetScale({4.2F, 1.6F, 1.0F});
        }
        waterTint->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Multiply);
        waterTint->AddComponent<Spark::SpriteComponent>(
                checkerTex,
                Spark::Vector4{0.58F, 0.82F, 0.98F, 0.78F},
                Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                50);
        roots.PushBack(waterTint);

        Spark::GameObject* waterGlint = w.CreateGameObject();
        waterGlint->GetName() = Spark::Utf8String("WaterGlint");
        {
            Spark::TransformComponent* tr = waterGlint->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({7.0F, 2.35F, 0.02F});
            tr->SetScale({2.4F, 0.35F, 1.0F});
        }
        waterGlint->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
        waterGlint->AddComponent<Spark::SpriteComponent>(
                checkerTex,
                Spark::Vector4{0.75F, 0.95F, 1.0F, 0.55F},
                Spark::Vector4{0.25F, 0.25F, 0.75F, 0.75F},
                55);
        roots.PushBack(waterGlint);

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("TwoDFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(12.0F, 12.0F);
        fpsText->SetFontSizePixels(20.0F);
        fpsText->SetColor({0.92F, 0.98F, 0.95F});
        fpsText->SetText(Spark::Utf8String("2D — Camera2D · blend modes (multiply water, additive glint)"));
        roots.PushBack(fpsHudObject);

        camera.position = {static_cast<float>(kMapW) * 0.5F, static_cast<float>(kMapH) * 0.5F, 0.0F};
        camera.halfExtentY = 5.5F;
        camera.rotationRad = 0.0F;

        context.GetInput().SetCursorCaptured(false);
    }

void TwoDDemo::Unload(Spark::GameWorld& w)
{
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        checkerTex.Reset();
        mapObject = nullptr;
        spriteRed = nullptr;
        spriteGreen = nullptr;
        spriteRedTr = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
    }

void TwoDDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        Spark::IInput& in = context.GetInput();
        const float dt = timing.deltaTimeSeconds;
        const float pan = 9.0F * dt;
        if (in.IsKeyDown(GLFW_KEY_W)) {
            camera.position.y += pan;
        }
        if (in.IsKeyDown(GLFW_KEY_S)) {
            camera.position.y -= pan;
        }
        if (in.IsKeyDown(GLFW_KEY_A)) {
            camera.position.x -= pan;
        }
        if (in.IsKeyDown(GLFW_KEY_D)) {
            camera.position.x += pan;
        }
        if (in.IsKeyDown(GLFW_KEY_COMMA)) {
            camera.rotationRad -= 1.35F * dt;
        }
        if (in.IsKeyDown(GLFW_KEY_PERIOD)) {
            camera.rotationRad += 1.35F * dt;
        }
        float hz = camera.halfExtentY;
        if (in.IsKeyDown(GLFW_KEY_LEFT_BRACKET)) {
            hz *= 1.0F - 1.15F * dt;
        }
        if (in.IsKeyDown(GLFW_KEY_RIGHT_BRACKET)) {
            hz *= 1.0F + 1.15F * dt;
        }
        camera.halfExtentY = std::clamp(hz, 2.0F, 40.0F);

        if (spriteRedTr != nullptr) {
            const float bob = std::sin(timing.totalTimeSeconds * 3.2F) * 0.12F;
            spriteRedTr->SetTranslation({4.5F, 3.5F + bob, 0.02F});
        }

        if (fpsText != nullptr) {
            const float tdt = timing.deltaTimeSeconds;
            const float instant = (tdt > 1.0e-6F) ? (1.0F / tdt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            fpsText->SetText(Spark::Utf8String(
                    std::format(
                            "2D — {:.0f} FPS — WASD pan · , . rotate · [ ] zoom · ESC menu",
                            static_cast<double>(fpsSmoothed))
                            .c_str()));
        }
    }

void TwoDDemo::Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context)
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
                Spark::Vector3{0.32F, 0.85F, 0.42F}.Normalized(),
                Spark::Vector3{1.0F, 1.0F, 1.0F},
                0.85F,
                Spark::Vector3{0.22F, 0.24F, 0.28F},
                false,
                pr,
                pu,
                0.0F);
    }
}  // namespace Spark
