#include "spark/demo/ParticleDemo.hpp"

namespace Spark {

namespace {

struct ParticleDemoBinding {
    ParticleDemo* demo = nullptr;
};

void BindFloatField(void* userData, const float value) {
    if (userData != nullptr) {
        *static_cast<float*>(userData) = value;
    }
}

void BindBoolField(void* userData, const bool value) {
    if (userData != nullptr) {
        *static_cast<bool*>(userData) = value;
    }
}

Ui::UiFloatCallback MakeFloatBinding(float* field) {
    Ui::UiFloatCallback callback{};
    callback.fn = &BindFloatField;
    callback.userData = field;
    return callback;
}

Ui::UiBoolCallback MakeBoolBinding(bool* field) {
    Ui::UiBoolCallback callback{};
    callback.fn = &BindBoolField;
    callback.userData = field;
    return callback;
}

void EmitterListSelected(void* userData, const int index) {
    if (userData == nullptr) {
        return;
    }
    auto* demo = static_cast<ParticleDemoBinding*>(userData)->demo;
    if (demo == nullptr) {
        return;
    }
    demo->OnEmitterListSelected(index);
}

void ResetPresetClicked(void* userData) {
    if (userData == nullptr) {
        return;
    }
    auto* demo = static_cast<ParticleDemoBinding*>(userData)->demo;
    if (demo == nullptr) {
        return;
    }
    demo->OnResetPresetClicked();
}

void AddSlider(
        Ui::IUiElement& parent,
        Ui::IUiControlsFactory& factory,
        const char* id,
        const char* label,
        float* valueField,
        Ui::ISlider** outSlider,
        const float minValue,
        const float maxValue) {
    Ui::SliderDesc desc{};
    desc.id = Utf8String(id);
    desc.label = Utf8String(label);
    desc.value = *valueField;
    desc.minValue = minValue;
    desc.maxValue = maxValue;
    auto slider = factory.CreateSlider(desc);
    slider->SetOnChanged(MakeFloatBinding(valueField));
    if (outSlider != nullptr) {
        *outSlider = slider.Get();
    }
    AdoptUiChild(parent, MoveTemp(slider));
}

}  // namespace

void ParticleDemo::OnEmitterListSelected(const int index) {
    guiSelectedEffect = index;
    SyncTuningFromEmitter();
}

void ParticleDemo::OnResetPresetClicked() {
    if (ParticleEmitterComponent* pe = SelectedEmitter(); pe != nullptr) {
        Detail::ApplyParticlePreset(guiSelectedEffect, *pe);
        SyncTuningFromEmitter();
    }
}

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
        fpsText->SetText(Spark::Utf8String("Particles — Fire · Snow · Smoke · Magic — UI panel"));
        roots.PushBack(fpsHudObject);

        guiSelectedEffect = 0;
        BuildRetainedUi(w);
        SyncTuningFromEmitter();

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
        uiCanvas = nullptr;
        uiRoot = nullptr;
        uiEmitterList = nullptr;
        uiSliderEmission = nullptr;
        uiSliderLifeMin = nullptr;
        uiSliderLifeMax = nullptr;
        uiSliderSizeStart = nullptr;
        uiSliderSizeEnd = nullptr;
        uiSliderSpread = nullptr;
        uiSliderSpeedMin = nullptr;
        uiSliderSpeedMax = nullptr;
        uiSliderGravY = nullptr;
        uiCheckboxEnabled = nullptr;
    }

void ParticleDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        ApplyTuningToEmitter();
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

        PaintUiCanvases(world, params, fbW, fbH);
        context.SetSceneRenderParams(params);
    }

Spark::ParticleEmitterComponent* ParticleDemo::SelectedEmitter() noexcept
{
        if (guiSelectedEffect < 0 || guiSelectedEffect >= Detail::kParticleDemoEffectCount) {
            return nullptr;
        }
        return effectEmitters[static_cast<std::size_t>(guiSelectedEffect)];
    }

void ParticleDemo::SyncTuningFromEmitter() noexcept
{
        ParticleEmitterComponent* pe = SelectedEmitter();
        if (pe == nullptr) {
            return;
        }
        guiEmission = pe->GetEmissionRate();
        guiLifeMin = pe->GetLifetimeMin();
        guiLifeMax = pe->GetLifetimeMax();
        guiSizeStart = pe->GetStartSize();
        guiSizeEnd = pe->GetEndSize();
        guiSpread = pe->GetSpreadAngleRadians();
        guiSpeedMin = pe->GetSpeedMin();
        guiSpeedMax = pe->GetSpeedMax();
        guiGravY = pe->GetGravity().y;
        guiEnabled = pe->IsEmitterEnabled();

        if (uiEmitterList != nullptr) {
            uiEmitterList->SetSelectedIndex(guiSelectedEffect);
        }
        if (uiSliderEmission != nullptr) {
            uiSliderEmission->SetValue(guiEmission);
        }
        if (uiSliderLifeMin != nullptr) {
            uiSliderLifeMin->SetValue(guiLifeMin);
        }
        if (uiSliderLifeMax != nullptr) {
            uiSliderLifeMax->SetValue(guiLifeMax);
        }
        if (uiSliderSizeStart != nullptr) {
            uiSliderSizeStart->SetValue(guiSizeStart);
        }
        if (uiSliderSizeEnd != nullptr) {
            uiSliderSizeEnd->SetValue(guiSizeEnd);
        }
        if (uiSliderSpread != nullptr) {
            uiSliderSpread->SetValue(guiSpread);
        }
        if (uiSliderSpeedMin != nullptr) {
            uiSliderSpeedMin->SetValue(guiSpeedMin);
        }
        if (uiSliderSpeedMax != nullptr) {
            uiSliderSpeedMax->SetValue(guiSpeedMax);
        }
        if (uiSliderGravY != nullptr) {
            uiSliderGravY->SetValue(guiGravY);
        }
        if (uiCheckboxEnabled != nullptr) {
            uiCheckboxEnabled->SetValue(guiEnabled);
        }
    }

void ParticleDemo::ApplyTuningToEmitter() noexcept
{
        ParticleEmitterComponent* pe = SelectedEmitter();
        if (pe == nullptr) {
            return;
        }
        pe->SetEmissionRate(guiEmission);
        pe->SetLifetime(guiLifeMin, std::max(guiLifeMin, guiLifeMax));
        pe->SetStartEndSize(guiSizeStart, guiSizeEnd);
        pe->SetSpreadAngleRadians(guiSpread);
        pe->SetSpeedRange(guiSpeedMin, std::max(guiSpeedMin, guiSpeedMax));
        const Vector3 g = pe->GetGravity();
        pe->SetGravity({g.x, guiGravY, g.z});
        pe->SetEmitterEnabled(guiEnabled);
    }

void ParticleDemo::BuildRetainedUi(Spark::GameWorld& world) {
    if (uiRoot != nullptr) {
        world.DestroyGameObject(uiRoot);
        uiRoot = nullptr;
        uiCanvas = nullptr;
        uiEmitterList = nullptr;
        uiSliderEmission = nullptr;
        uiSliderLifeMin = nullptr;
        uiSliderLifeMax = nullptr;
        uiSliderSizeStart = nullptr;
        uiSliderSizeEnd = nullptr;
        uiSliderSpread = nullptr;
        uiSliderSpeedMin = nullptr;
        uiSliderSpeedMax = nullptr;
        uiSliderGravY = nullptr;
        uiCheckboxEnabled = nullptr;
    }
    uiRoot = world.CreateGameObject();
    uiRoot->GetName() = Spark::Utf8String("ParticleUi");
    uiCanvas = uiRoot->AddComponent<UiCanvasComponent>();
    uiCanvas->SetSortOrder(100);
    uiCanvas->SetTheme(Ui::UiTheme::ClassicMint());
    roots.PushBack(uiRoot);

    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    Ui::PanelDesc panelDesc{};
    panelDesc.id = Utf8String("particles");
    panelDesc.title = Utf8String("Particle effects");
    panelDesc.width = DemoGui::kDemoSidePanelWidth;
    panelDesc.height = 640.0F;
    panelDesc.anchorRight = true;
    panelDesc.edgeMargin = 12.0F;
    auto panel = factory.CreatePanel(panelDesc);

    Ui::LabelDesc emitterHdr{};
    emitterHdr.id = Utf8String("emitter_hdr");
    emitterHdr.text = Utf8String("Emitter");
    AdoptUiChild(*panel, factory.CreateLabel(emitterHdr));

    Ui::ListDesc listDesc{};
    listDesc.id = Utf8String("fx_list");
    listDesc.rowHeight = 28.0F;
    listDesc.verticalScrollingEnabled = true;
    auto list = factory.CreateList(listDesc);
    uiEmitterList = list.Get();
    static constexpr const char* kEmitterNames[Detail::kParticleDemoEffectCount] = {
            "Fire (campfire)", "Snow", "Smoke", "Magic sparkles"};
    Array<Utf8String> items;
    items.Reserve(static_cast<std::size_t>(Detail::kParticleDemoEffectCount));
    for (int ei = 0; ei < Detail::kParticleDemoEffectCount; ++ei) {
        items.PushBack(Utf8String(kEmitterNames[static_cast<std::size_t>(ei)]));
    }
    list->SetItems(MoveTemp(items));
    list->SetSelectedIndex(guiSelectedEffect);
    static ParticleDemoBinding listBinding{};
    listBinding.demo = this;
    Ui::UiIntCallback selectCb{};
    selectCb.fn = &EmitterListSelected;
    selectCb.userData = &listBinding;
    list->SetOnSelectionChanged(selectCb);
    AdoptUiChild(*panel, MoveTemp(list));

    Ui::SeparatorDesc sepDesc{};
    sepDesc.id = Utf8String("sep");
    AdoptUiChild(*panel, factory.CreateSeparator(sepDesc));

    Ui::LabelDesc helpDesc{};
    helpDesc.id = Utf8String("help");
    helpDesc.text = Utf8String("Tune the selected emitter; F1 toggles fly camera.");
    helpDesc.muted = true;
    AdoptUiChild(*panel, factory.CreateLabel(helpDesc));

    Ui::ScrollPanelDesc scrollDesc{};
    scrollDesc.id = Utf8String("particle_scroll");
    scrollDesc.height = 320.0F;
    auto scrollPanel = factory.CreateScrollPanel(scrollDesc);

    AddSlider(*scrollPanel, factory, "emission", "Emission / sec", &guiEmission, &uiSliderEmission, 0.0F, 320.0F);
    AddSlider(*scrollPanel, factory, "lmin", "Lifetime min (s)", &guiLifeMin, &uiSliderLifeMin, 0.05F, 4.0F);
    AddSlider(*scrollPanel, factory, "lmax", "Lifetime max (s)", &guiLifeMax, &uiSliderLifeMax, 0.1F, 5.0F);
    AddSlider(*scrollPanel, factory, "sz0", "Size start", &guiSizeStart, &uiSliderSizeStart, 0.02F, 0.55F);
    AddSlider(*scrollPanel, factory, "sz1", "Size end", &guiSizeEnd, &uiSliderSizeEnd, 0.01F, 0.55F);
    AddSlider(*scrollPanel, factory, "spread", "Spread (rad)", &guiSpread, &uiSliderSpread, 0.0F, 3.14159F);
    AddSlider(*scrollPanel, factory, "spmin", "Speed min", &guiSpeedMin, &uiSliderSpeedMin, 0.0F, 8.0F);
    AddSlider(*scrollPanel, factory, "spmax", "Speed max", &guiSpeedMax, &uiSliderSpeedMax, 0.0F, 12.0F);
    AddSlider(*scrollPanel, factory, "grav", "Gravity Y", &guiGravY, &uiSliderGravY, -12.0F, 8.0F);
    AdoptUiChild(*panel, MoveTemp(scrollPanel));

    Ui::CheckBoxDesc enabledDesc{};
    enabledDesc.id = Utf8String("enabled");
    enabledDesc.label = Utf8String("Emitter enabled");
    enabledDesc.value = guiEnabled;
    auto enabledBox = factory.CreateCheckBox(enabledDesc);
    uiCheckboxEnabled = enabledBox.Get();
    enabledBox->SetOnChanged(MakeBoolBinding(&guiEnabled));
    AdoptUiChild(*panel, MoveTemp(enabledBox));

    Ui::ButtonDesc resetDesc{};
    resetDesc.id = Utf8String("reset");
    resetDesc.label = Utf8String("Reset selected preset");
    auto resetBtn = factory.CreateButton(resetDesc);
    static ParticleDemoBinding resetBinding{};
    resetBinding.demo = this;
    Ui::UiVoidCallback resetCb{};
    resetCb.fn = &ResetPresetClicked;
    resetCb.userData = &resetBinding;
    resetBtn->SetOnClick(resetCb);
    AdoptUiChild(*panel, MoveTemp(resetBtn));

    uiCanvas->SetRoot(MoveTemp(panel));
}

}  // namespace Spark
