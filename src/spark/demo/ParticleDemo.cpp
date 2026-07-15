#include "spark/demo/ParticleDemo.hpp"

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
        fpsText->SetScreenPosition(12.0F, 12.0F);
        fpsText->SetFontSizePixels(20.0F);
        fpsText->SetColor({0.9F, 0.95F, 1.0F});
        fpsText->SetText(Spark::Utf8String("Particles — Fire · Snow · Smoke · Magic — GUI panel"));
        roots.PushBack(fpsHudObject);

        guiObject = w.CreateGameObject();
        guiObject->GetName() = Spark::Utf8String("ParticleDemoGui");
        Spark::GuiCanvasComponent* canvas = guiObject->AddComponent<Spark::GuiCanvasComponent>();
        canvas->SetSortOrder(240);
        BuildGuiPanel(*canvas);
        roots.PushBack(guiObject);

        guiSelectedEffect = 0;
        SyncGuiFromSelectedEmitter();

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
        guiObject = nullptr;
        ClearGuiWidgetRefs();
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

        Spark::PaintGuiCanvases(world, params, fbW, fbH);

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

void ParticleDemo::ClearGuiWidgetRefs() noexcept
{
        for (std::size_t i = 0; i < Detail::kParticleDemoEffectCount; ++i) {
            guiEffectButtons[i] = nullptr;
        }
        guiEmission = nullptr;
        guiLifeMin = nullptr;
        guiLifeMax = nullptr;
        guiSizeStart = nullptr;
        guiSizeEnd = nullptr;
        guiSpread = nullptr;
        guiSpeedMin = nullptr;
        guiSpeedMax = nullptr;
        guiGravY = nullptr;
        guiEnabledSwitch = nullptr;
    }

[[nodiscard]] Spark::ParticleEmitterComponent* ParticleDemo::SelectedEmitter() noexcept
{
        if (guiSelectedEffect < 0 || guiSelectedEffect >= Detail::kParticleDemoEffectCount) {
            return nullptr;
        }
        return effectEmitters[static_cast<std::size_t>(guiSelectedEffect)];
    }

void ParticleDemo::SyncGuiFromSelectedEmitter()
{
        for (int i = 0; i < Detail::kParticleDemoEffectCount; ++i) {
            if (guiEffectButtons[static_cast<std::size_t>(i)] != nullptr) {
                guiEffectButtons[static_cast<std::size_t>(i)]->SetAccentSelected(i == guiSelectedEffect);
            }
        }
        Spark::ParticleEmitterComponent* pe = SelectedEmitter();
        if (pe == nullptr) {
            return;
        }
        if (guiEmission != nullptr) {
            guiEmission->SetRange(0.0F, 320.0F);
            guiEmission->SetValue(pe->GetEmissionRate());
        }
        if (guiLifeMin != nullptr) {
            guiLifeMin->SetRange(0.05F, 4.0F);
            guiLifeMin->SetValue(pe->GetLifetimeMin());
        }
        if (guiLifeMax != nullptr) {
            guiLifeMax->SetRange(0.1F, 5.0F);
            guiLifeMax->SetValue(pe->GetLifetimeMax());
        }
        if (guiSizeStart != nullptr) {
            guiSizeStart->SetRange(0.02F, 0.55F);
            guiSizeStart->SetValue(pe->GetStartSize());
        }
        if (guiSizeEnd != nullptr) {
            guiSizeEnd->SetRange(0.01F, 0.55F);
            guiSizeEnd->SetValue(pe->GetEndSize());
        }
        if (guiSpread != nullptr) {
            guiSpread->SetRange(0.0F, 3.14159F);
            guiSpread->SetValue(pe->GetSpreadAngleRadians());
        }
        if (guiSpeedMin != nullptr) {
            guiSpeedMin->SetRange(0.0F, 8.0F);
            guiSpeedMin->SetValue(pe->GetSpeedMin());
        }
        if (guiSpeedMax != nullptr) {
            guiSpeedMax->SetRange(0.0F, 12.0F);
            guiSpeedMax->SetValue(pe->GetSpeedMax());
        }
        if (guiGravY != nullptr) {
            guiGravY->SetRange(-12.0F, 8.0F);
            guiGravY->SetValue(pe->GetGravity().y);
        }
        if (guiEnabledSwitch != nullptr) {
            guiEnabledSwitch->SetOn(pe->IsEmitterEnabled());
        }
    }

void ParticleDemo::BuildGuiPanel(Spark::GuiCanvasComponent& canvas)
{
        ClearGuiWidgetRefs();

        auto root = Spark::MakeUnique<ParticleEffectsRightDockRoot>();
        auto shell = Spark::MakeUnique<Spark::Gui::Panel>();
        shell->SetPadding(18.0F);
        shell->SetChromeEnabled(true);
        shell->SetDropShadowEnabled(true);
        shell->SetBackgroundGradient(
                Spark::Vector3{0.14F, 0.16F, 0.22F}, Spark::Vector3{0.08F, 0.09F, 0.13F}, 0.97F);

        auto layout = Spark::MakeUnique<ParticleGuiEmitterOverlayLayout>();

        auto lowerStack = Spark::MakeUnique<Spark::Gui::StackPanel>();
        lowerStack->SetOrientation(Spark::Gui::StackOrientation::Vertical);
        lowerStack->SetSpacing(9.0F);

        auto title = Spark::MakeUnique<Spark::Gui::Label>();
        title->SetText(Spark::Utf8String("Particle effects"));
        title->SetFontSize(30.0F);
        title->SetBold(true);
        title->SetTextColor({0.94F, 0.96F, 1.0F});

        ParticleDemo* self = this;

        auto pickLbl = Spark::MakeUnique<Spark::Gui::Label>();
        pickLbl->SetText(Spark::Utf8String("Emitter"));
        pickLbl->SetFontSize(21.0F);
        pickLbl->SetTextColor({0.86F, 0.90F, 0.95F});

        auto emitterPick = Spark::MakeUnique<Spark::Gui::StackPanel>();
        emitterPick->SetOrientation(Spark::Gui::StackOrientation::Vertical);
        emitterPick->SetSpacing(6.0F);
        static constexpr const char* kEmitterNames[Detail::kParticleDemoEffectCount] = {
                "Fire (campfire)", "Snow", "Smoke", "Magic sparkles"};
        for (int ei = 0; ei < Detail::kParticleDemoEffectCount; ++ei) {
            auto eb = Spark::MakeUnique<Spark::Gui::Button>();
            guiEffectButtons[static_cast<std::size_t>(ei)] = eb.Get();
            eb->SetLabel(Spark::Utf8String(kEmitterNames[static_cast<std::size_t>(ei)]));
            eb->SetFontSize(19.0F);
            eb->SetOpaqueSurface(true);
            const int preset = ei;
            eb->SetOnClick([self, preset]() {
                self->guiSelectedEffect = preset;
                self->SyncGuiFromSelectedEmitter();
            });
            emitterPick->AddChild(Spark::MoveTemp(eb));
        }

        auto help = Spark::MakeUnique<Spark::Gui::WrappingLabel>();
        help->SetText(Spark::Utf8String(
                "Choose an emitter preset above, then tune sliders or reset. "
                "F1 toggles fly camera vs mouse for this panel."));
        help->SetFontSize(21.0F);
        help->SetTextColor({0.82F, 0.86F, 0.92F});
        lowerStack->AddChild(Spark::MoveTemp(help));

        auto addLabeledSlider =
                [&lowerStack](const char* title, float r0, float r1, Spark::Gui::Slider*& outPtr) {
                    auto row = Spark::MakeUnique<Spark::Gui::StackPanel>();
                    row->SetOrientation(Spark::Gui::StackOrientation::Horizontal);
                    row->SetSpacing(10.0F);
                    auto lab = Spark::MakeUnique<Spark::Gui::Label>();
                    lab->SetText(Spark::Utf8String(title));
                    lab->SetFontSize(20.0F);
                    lab->SetTextColor({0.86F, 0.90F, 0.95F});
                    auto sl = Spark::MakeUnique<Spark::Gui::Slider>();
                    outPtr = sl.Get();
                    sl->SetRange(r0, r1);
                    row->AddChild(Spark::MoveTemp(lab));
                    row->AddChild(Spark::MoveTemp(sl));
                    lowerStack->AddChild(Spark::MoveTemp(row));
                };

        addLabeledSlider("Emission / sec", 0.0F, 320.0F, guiEmission);
        addLabeledSlider("Lifetime min (s)", 0.05F, 4.0F, guiLifeMin);
        addLabeledSlider("Lifetime max (s)", 0.1F, 5.0F, guiLifeMax);
        addLabeledSlider("Size start", 0.02F, 0.55F, guiSizeStart);
        addLabeledSlider("Size end", 0.01F, 0.55F, guiSizeEnd);
        addLabeledSlider("Spread (rad)", 0.0F, 3.14159F, guiSpread);
        addLabeledSlider("Speed min", 0.0F, 8.0F, guiSpeedMin);
        addLabeledSlider("Speed max", 0.0F, 12.0F, guiSpeedMax);
        addLabeledSlider("Gravity Y", -12.0F, 8.0F, guiGravY);

        if (guiEmission != nullptr) {
            guiEmission->SetOnChanged([self](float v) {
                if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                    pe->SetEmissionRate(v);
                }
            });
        }
        if (guiLifeMin != nullptr) {
            guiLifeMin->SetOnChanged([self](float v) {
                if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                    pe->SetLifetime(v, std::max(v, pe->GetLifetimeMax()));
                }
            });
        }
        if (guiLifeMax != nullptr) {
            guiLifeMax->SetOnChanged([self](float v) {
                if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                    pe->SetLifetime(std::min(pe->GetLifetimeMin(), v), v);
                }
            });
        }
        if (guiSizeStart != nullptr) {
            guiSizeStart->SetOnChanged([self](float v) {
                if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                    pe->SetStartEndSize(v, pe->GetEndSize());
                }
            });
        }
        if (guiSizeEnd != nullptr) {
            guiSizeEnd->SetOnChanged([self](float v) {
                if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                    pe->SetStartEndSize(pe->GetStartSize(), v);
                }
            });
        }
        if (guiSpread != nullptr) {
            guiSpread->SetOnChanged([self](float v) {
                if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                    pe->SetSpreadAngleRadians(v);
                }
            });
        }
        if (guiSpeedMin != nullptr) {
            guiSpeedMin->SetOnChanged([self](float v) {
                if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                    pe->SetSpeedRange(v, std::max(v, pe->GetSpeedMax()));
                }
            });
        }
        if (guiSpeedMax != nullptr) {
            guiSpeedMax->SetOnChanged([self](float v) {
                if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                    pe->SetSpeedRange(std::min(pe->GetSpeedMin(), v), v);
                }
            });
        }
        if (guiGravY != nullptr) {
            guiGravY->SetOnChanged([self](float v) {
                if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                    const Spark::Vector3 g = pe->GetGravity();
                    pe->SetGravity({g.x, v, g.z});
                }
            });
        }

        auto swRow = Spark::MakeUnique<Spark::Gui::StackPanel>();
        swRow->SetOrientation(Spark::Gui::StackOrientation::Horizontal);
        swRow->SetSpacing(12.0F);
        auto swLab = Spark::MakeUnique<Spark::Gui::Label>();
        swLab->SetText(Spark::Utf8String("Emitter enabled"));
        swLab->SetFontSize(21.0F);
        swLab->SetTextColor({0.86F, 0.90F, 0.95F});
        auto sw = Spark::MakeUnique<Spark::Gui::Switch>();
        guiEnabledSwitch = sw.Get();
        sw->SetOnChanged([self](bool on) {
            if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                pe->SetEmitterEnabled(on);
            }
        });
        swRow->AddChild(Spark::MoveTemp(swLab));
        swRow->AddChild(Spark::MoveTemp(sw));
        lowerStack->AddChild(Spark::MoveTemp(swRow));

        auto resetBtn = Spark::MakeUnique<Spark::Gui::Button>();
        resetBtn->SetLabel(Spark::Utf8String("Reset selected preset"));
        resetBtn->SetFontSize(23.0F);
        resetBtn->SetOnClick([self]() {
            if (Spark::ParticleEmitterComponent* pe = self->SelectedEmitter()) {
                Detail::ApplyParticlePreset(self->guiSelectedEffect, *pe);
                self->SyncGuiFromSelectedEmitter();
            }
        });
        lowerStack->AddChild(Spark::MoveTemp(resetBtn));

        layout->AddChild(Spark::MoveTemp(lowerStack));
        layout->AddChild(Spark::MoveTemp(title));
        layout->AddChild(Spark::MoveTemp(pickLbl));
        layout->AddChild(Spark::MoveTemp(emitterPick));
        shell->AddChild(Spark::MoveTemp(layout));
        root->AddChild(Spark::MoveTemp(shell));
        canvas.SetRoot(Spark::MoveTemp(root));
    }
}  // namespace Spark
