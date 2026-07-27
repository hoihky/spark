#include "spark/demo/ParticleDemo.hpp"

#include "spark/demo/DemoGuiFrame.hpp"
#include "spark/gui/api/GuiApi.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"

namespace Spark {

void ParticleDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("ParticleDemoGround"));
        *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent);
        groundObject = w.CreateGameObject();
        groundObject->GetName() = Spark::Utf8String("Ground");
        groundObject->AddComponent<Spark::TransformComponent>();
        groundObject->AddComponent<Spark::MeshComponent>(
                groundAsset, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3{0.35F, 0.4F, 0.36F});
        roots.PushBack(groundObject);

        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("ParticleDemoCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        cubeObject = w.CreateGameObject();
        cubeObject->GetName() = Spark::Utf8String("RefCube");
        {
            Spark::TransformComponent* tr = cubeObject->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({1.8F, 0.85F, -1.2F});
            tr->SetUniformScale(0.9F);
        }
        cubeObject->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.75F, 0.5F, 0.22F});
        roots.PushBack(cubeObject);

        static constexpr float kFxPos[Detail::kParticleDemoEffectCount][3] = {
                {0.0F, 0.16F, 0.0F},
                {0.0F, 5.6F, 0.0F},
                {-2.35F, 0.22F, 0.95F},
                {2.45F, 0.28F, -1.25F},
        };
        static constexpr const char* kFxNames[Detail::kParticleDemoEffectCount] = {
                "FireEmitter",
                "SnowEmitter",
                "SmokeEmitter",
                "MagicEmitter",
        };
        for (int i = 0; i < Detail::kParticleDemoEffectCount; ++i) {
            Spark::GameObject* go = w.CreateGameObject();
            go->GetName() = Spark::Utf8String(kFxNames[static_cast<std::size_t>(i)]);
            Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation(
                    {kFxPos[static_cast<std::size_t>(i)][0],
                     kFxPos[static_cast<std::size_t>(i)][1],
                     kFxPos[static_cast<std::size_t>(i)][2]});
            Spark::ParticleEmitterComponent* pe = go->AddComponent<Spark::ParticleEmitterComponent>();
            Detail::ApplyParticlePreset(i, *pe);
            effectObjects[static_cast<std::size_t>(i)] = go;
            effectEmitters[static_cast<std::size_t>(i)] = pe;
            roots.PushBack(go);
        }

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("ParticleFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*fpsText);
        fpsText->SetText(Spark::Utf8String("Particles — Fire · Snow · Smoke · Magic — GUI panel"));
        roots.PushBack(fpsHudObject);

        guiSelectedEffect = 0;

        context.GetInput().SetCursorCaptured(false);
        camera.position = {0.0F, 3.2F, 10.0F};
        camera.SnapLookAt({0.0F, 1.0F, 0.0F});
    }

void ParticleDemo::Unload(Spark::GameWorld& w)
{
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        groundObject = nullptr;
        cubeObject = nullptr;
        for (int i = 0; i < Detail::kParticleDemoEffectCount; ++i) {
            effectObjects[static_cast<std::size_t>(i)] = nullptr;
            effectEmitters[static_cast<std::size_t>(i)] = nullptr;
        }
        fpsHudObject = nullptr;
        fpsText = nullptr;
    }

void ParticleDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        Spark::IInput& in = context.GetInput();
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (fpsText != nullptr) {
            const float dt = timing.deltaTimeSeconds;
            const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            fpsText->SetText(Spark::Utf8String(
                    std::format(
                            "Particles — {:.0f} FPS — F1: {} (use panel when mouse is free)",
                            static_cast<double>(fpsSmoothed),
                            in.IsCursorCaptured() ? "release mouse" : "fly camera")
                            .c_str()));
        }
    }

void ParticleDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

        const Spark::Matrix4 proj =
                Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 400.0F);
        const Spark::Matrix4 view = camera.ViewMatrix();
        const Spark::Matrix4 viewProj = proj * view;

        Spark::SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = camera.position;
        params.lightDirectionWorld = Spark::Vector3{0.4F, 0.85F, 0.2F}.Normalized();
        params.lightColor = {1.0F, 0.98F, 0.92F};
        params.lightIntensity = 0.92F;
        params.ambientColor = {0.10F, 0.11F, 0.14F};

        const Spark::Vector3 f = camera.Forward();
        Spark::Vector3 r = Spark::Vector3::Cross(Spark::Vector3::UnitY, f);
        if (r.LengthSquared() < 1.0e-10F) {
            r = Spark::Vector3::UnitX;
        } else {
            r = r.Normalized();
        }
        const Spark::Vector3 u = Spark::Vector3::Cross(f, r).Normalized();
        params.particleCameraRight = r;
        params.particleCameraUp = u;

        params.particles.Clear();
        scene.ForEachParticleEmitter([&params](const Spark::ParticleEmitterComponent& pe,
                                                    const Spark::Matrix4& /*world*/) {
            Spark::Array<Spark::SceneParticleInstance> chunk;
            pe.CollectInstances(chunk);
            for (std::size_t ci = 0; ci < chunk.GetSize(); ++ci) {
                if (params.particles.GetSize() >= Spark::SceneRenderParams::MaxParticles) {
                    return;
                }
                params.particles.PushBack(chunk[ci]);
            }
        });

        params.draws.Clear();
        params.sceneTextures.Clear();
        params.pointLights.Clear();
        params.sprites.Clear();
        params.screenRects.Clear();
        params.screenTexts.Clear();
        params.screenOverlayRects.Clear();
        params.screenOverlayTexts.Clear();
        params.screenLateRects.Clear();
        params.screenLateTexts.Clear();
        params.uiFont = world.GetUiFont();
        params.uiBoldFont = world.GetUiBoldFont();
        params.draws.Reserve(16);

        scene.ForEachDrawable([&](Spark::GameObject* obj, const Spark::MeshComponent& mc,
                                     const Spark::MaterialComponent* mat, const Spark::Matrix4& world) {
            if (obj != nullptr && obj->GetComponent<Spark::SkyComponent>() != nullptr) {
                return;
            }
            Spark::SceneDrawItem item{};
            item.model = world;
            item.mesh = mc.GetSlot();
            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom) {
                item.customMesh = mc.GetMesh();
            }
            Spark::Vector3 alb = mc.GetAlbedo();
            item.textureLayer = -1;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
            }
            item.albedo = alb;
            params.draws.PushBack(item);
        });

        BuildPortableUi(context, params, world);

        scene.ForEachTextOverlay([&params](const Spark::TextOverlayComponent& tc) {
            Spark::ScreenTextDraw d{};
            d.text = tc.GetText();
            d.x = tc.GetScreenX();
            d.y = tc.GetScreenY();
            d.sizePixels = tc.GetFontSizePixels();
            d.color = tc.GetColor();
            d.alpha = tc.GetAlpha();
            d.paintOrder = params.NextUiPaintOrder();
            params.screenTexts.PushBack(Spark::MoveTemp(d));
        });
        context.SetSceneRenderParams(params);
    }

Spark::ParticleEmitterComponent* ParticleDemo::SelectedEmitter() noexcept
{
        if (guiSelectedEffect < 0 || guiSelectedEffect >= Detail::kParticleDemoEffectCount) {
            return nullptr;
        }
        return effectEmitters[static_cast<std::size_t>(guiSelectedEffect)];
    }

void ParticleDemo::BuildPortableUi(
        Spark::IEngineContext& context,
        SceneRenderParams& params,
        const Spark::GameWorld& world) {
    Spark::ParticleEmitterComponent* pe = SelectedEmitter();
    if (pe == nullptr) {
        return;
    }

    float emission = pe->GetEmissionRate();
    float lifeMin = pe->GetLifetimeMin();
    float lifeMax = pe->GetLifetimeMax();
    float sizeStart = pe->GetStartSize();
    float sizeEnd = pe->GetEndSize();
    float spread = pe->GetSpreadAngleRadians();
    float speedMin = pe->GetSpeedMin();
    float speedMax = pe->GetSpeedMax();
    float gravY = pe->GetGravity().y;
    bool enabled = pe->IsEmitterEnabled();

    const Gui::GuiFrameContext frame = DemoGui::MakeFrameContext(context, params, world, 0.0F);
    Gui::GuiSystem::Get().BeginImmediateFrame(frame);
    Gui::IGuiFrame& ui = Gui::Ui();

    const Gui::GuiLayoutMetrics& layout = Gui::GetActiveGuiLayoutMetrics();
    const float panelW = DemoGui::kDemoSidePanelWidth * layout.uiScale;
    const float panelX = static_cast<float>((std::max)(1, frame.framebufferWidth)) - panelW - layout.Padding();
    const float panelY = layout.Padding();
    const float panelH =
            static_cast<float>((std::max)(1, frame.framebufferHeight)) - panelY * 2.0F;
    ui.SetNextPanelSize(panelW, panelH);
    ui.SetCursorPos(panelX, panelY);
    if (ui.BeginPanel("particles", "Particle effects")) {
        ui.Text("Emitter");
        static constexpr const char* kEmitterNames[Detail::kParticleDemoEffectCount] = {
                "Fire (campfire)", "Snow", "Smoke", "Magic sparkles"};
        for (int ei = 0; ei < Detail::kParticleDemoEffectCount; ++ei) {
            char idBuf[32];
            snprintf(idBuf, sizeof(idBuf), "fx_%d", ei);
            if (ui.Selectable(idBuf, kEmitterNames[ei], guiSelectedEffect == ei)) {
                guiSelectedEffect = ei;
                pe = SelectedEmitter();
                if (pe != nullptr) {
                    emission = pe->GetEmissionRate();
                    lifeMin = pe->GetLifetimeMin();
                    lifeMax = pe->GetLifetimeMax();
                    sizeStart = pe->GetStartSize();
                    sizeEnd = pe->GetEndSize();
                    spread = pe->GetSpreadAngleRadians();
                    speedMin = pe->GetSpeedMin();
                    speedMax = pe->GetSpeedMax();
                    gravY = pe->GetGravity().y;
                    enabled = pe->IsEmitterEnabled();
                }
            }
        }
        ui.Separator();
        ui.TextDisabled("Tune the selected emitter; F1 toggles fly camera.");
        if (ui.SliderFloat("emission", "Emission / sec", emission, 0.0F, 320.0F)) {
            pe->SetEmissionRate(emission);
        }
        if (ui.SliderFloat("lmin", "Lifetime min (s)", lifeMin, 0.05F, 4.0F)) {
            pe->SetLifetime(lifeMin, std::max(lifeMin, pe->GetLifetimeMax()));
        }
        if (ui.SliderFloat("lmax", "Lifetime max (s)", lifeMax, 0.1F, 5.0F)) {
            pe->SetLifetime(std::min(pe->GetLifetimeMin(), lifeMax), lifeMax);
        }
        if (ui.SliderFloat("sz0", "Size start", sizeStart, 0.02F, 0.55F)) {
            pe->SetStartEndSize(sizeStart, pe->GetEndSize());
        }
        if (ui.SliderFloat("sz1", "Size end", sizeEnd, 0.01F, 0.55F)) {
            pe->SetStartEndSize(pe->GetStartSize(), sizeEnd);
        }
        if (ui.SliderFloat("spread", "Spread (rad)", spread, 0.0F, 3.14159F)) {
            pe->SetSpreadAngleRadians(spread);
        }
        if (ui.SliderFloat("spmin", "Speed min", speedMin, 0.0F, 8.0F)) {
            pe->SetSpeedRange(speedMin, std::max(speedMin, pe->GetSpeedMax()));
        }
        if (ui.SliderFloat("spmax", "Speed max", speedMax, 0.0F, 12.0F)) {
            pe->SetSpeedRange(std::min(pe->GetSpeedMin(), speedMax), speedMax);
        }
        if (ui.SliderFloat("grav", "Gravity Y", gravY, -12.0F, 8.0F)) {
            const Vector3 g = pe->GetGravity();
            pe->SetGravity({g.x, gravY, g.z});
        }
        if (ui.Checkbox("enabled", "Emitter enabled", enabled)) {
            pe->SetEmitterEnabled(enabled);
        }
        if (ui.Button("reset", "Reset selected preset")) {
            Detail::ApplyParticlePreset(guiSelectedEffect, *pe);
        }
        ui.EndPanel();
    }
    Gui::GuiSystem::Get().EndImmediateFrame();
}
}  // namespace Spark
