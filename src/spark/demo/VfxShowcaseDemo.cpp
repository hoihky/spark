#include "spark/demo/VfxShowcaseDemo.hpp"

#include "spark/config.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"
#include "spark/scene/vfx/VfxAssetLoader.hpp"
#include "spark/scene/vfx/VfxSubsystemProcess.hpp"
#include "spark/scene/editor/SceneEditorCameraController.hpp"
#include "spark/ui/runtime/UiScene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>
#include <sys/stat.h>

namespace Spark {

namespace {

struct VfxShowcaseBinding {
    VfxShowcaseDemo* demo = nullptr;
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

void EffectListSelected(void* userData, const int index) {
    if (userData == nullptr) {
        return;
    }
    auto* demo = static_cast<VfxShowcaseBinding*>(userData)->demo;
    if (demo != nullptr) {
        demo->OnEffectListSelected(index);
    }
}

void ResetPresetClicked(void* userData) {
    if (userData == nullptr) {
        return;
    }
    auto* demo = static_cast<VfxShowcaseBinding*>(userData)->demo;
    if (demo != nullptr) {
        demo->OnResetPresetClicked();
    }
}

void PlayEffectClicked(void* userData) {
    if (userData == nullptr) {
        return;
    }
    auto* demo = static_cast<VfxShowcaseBinding*>(userData)->demo;
    if (demo != nullptr) {
        demo->OnPlayEffectClicked();
    }
}

void SaveEffectClicked(void* userData) {
    if (userData == nullptr) {
        return;
    }
    auto* demo = static_cast<VfxShowcaseBinding*>(userData)->demo;
    if (demo != nullptr) {
        demo->OnSaveEffectClicked();
    }
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

const char* ModuleIdFromIndex(const int index) noexcept {
    switch (index) {
    case 1:
        return "burst_only";
    case 2:
        return "ring";
    default:
        return "continuous";
    }
}

int ModuleIndexFromId(const char* moduleId) noexcept {
    if (moduleId == nullptr) {
        return 0;
    }
    if (std::strcmp(moduleId, "burst_only") == 0) {
        return 1;
    }
    if (std::strcmp(moduleId, "ring") == 0) {
        return 2;
    }
    return 0;
}

void SaveNameCommitted(void* userData) {
    if (userData == nullptr) {
        return;
    }
    static_cast<VfxShowcaseDemo*>(userData)->OnSaveNameCommitted();
}

void ModuleSliderChanged(void* userData, const float value) {
    if (userData == nullptr) {
        return;
    }
    static_cast<VfxShowcaseDemo*>(userData)->OnModuleSliderChanged(value);
}

void OrbitCameraAroundPivot(
        FlyCamera& cam,
        const Vector3& pivot,
        float& orbitDistance,
        const float deltaX,
        const float deltaY) noexcept {
    cam.AddLook(deltaX, deltaY);
    orbitDistance = std::max(1.5F, orbitDistance);
    const Vector3 forward = cam.Forward();
    cam.position = {
            pivot.x - forward.x * orbitDistance,
            pivot.y - forward.y * orbitDistance,
            pivot.z - forward.z * orbitDistance};
}

void PanCamera(FlyCamera& cam, const float deltaX, const float deltaY, const float panScale) noexcept {
    const Vector3 forward = cam.Forward();
    Vector3 right = Vector3::Cross(forward, Vector3::UnitY);
    if (right.LengthSquared() < 1.0e-10F) {
        right = Vector3::UnitX;
    } else {
        right = right.Normalized();
    }
    const Vector3 up = Vector3::Cross(right, forward).Normalized();
    cam.position -= right * (deltaX * panScale) + up * (deltaY * panScale);
}

}  // namespace

const VfxShowcaseDetail::Entry& VfxShowcaseDemo::SelectedEntry() const noexcept {
    const int idx =
            guiSelectedEffect >= 0 && guiSelectedEffect < VfxShowcaseDetail::kEntryCount ? guiSelectedEffect : 0;
    return VfxShowcaseDetail::kCatalog[idx];
}

void VfxShowcaseDemo::ResetCamera() noexcept {
    cameraController.orbitPivot = previewPosition;
    cameraController.camera.position = {1.05F, 1.45F, 3.5F};
    cameraController.camera.SnapLookAt(previewPosition);
    const Vector3 offset{
            cameraController.camera.position.x - previewPosition.x,
            cameraController.camera.position.y - previewPosition.y,
            cameraController.camera.position.z - previewPosition.z};
    cameraController.orbitDistance = std::max(1.5F, offset.Length());
}

void VfxShowcaseDemo::SetStatusMessage(const char* message) {
    guiStatus = Utf8String(message != nullptr ? message : "");
    if (uiStatusLabel != nullptr) {
        uiStatusLabel->SetText(guiStatus);
    }
}

void VfxShowcaseDemo::OnEffectListSelected(const int index) {
    guiSelectedEffect = index;
    ApplySelectedPresetToPreview();
    SyncTuningFromEmitter();
}

void VfxShowcaseDemo::OnResetPresetClicked() {
    ApplySelectedPresetToPreview();
    SyncTuningFromEmitter();
    SetStatusMessage("Preset reset.");
}

void VfxShowcaseDemo::OnPlayEffectClicked() {
    if (activeWorld != nullptr) {
        PlaySelectedEffect(*activeWorld);
    }
}

void VfxShowcaseDemo::OnModuleSliderChanged(const float value) {
    guiModuleSlider = value;
    guiEmissionModule = static_cast<int>(value + 0.5F);
    if (guiEmissionModule < 0) {
        guiEmissionModule = 0;
    }
    if (guiEmissionModule > 2) {
        guiEmissionModule = 2;
    }
}

void VfxShowcaseDemo::OnSaveNameCommitted() {
    if (uiSaveNameBox != nullptr) {
        guiSaveName = Utf8String(uiSaveNameBox->GetText().CStr());
    }
}

void VfxShowcaseDemo::OnSaveEffectClicked() {
    OnSaveNameCommitted();
    if (previewEmitter == nullptr) {
        SetStatusMessage("No preview emitter.");
        return;
    }
    Utf8String name = guiSaveName;
    if (name.IsEmpty()) {
        SetStatusMessage("Enter an effect name first.");
        return;
    }
    for (std::size_t i = 0; i < name.ByteLength(); ++i) {
        const char ch = name.CStr()[i];
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')) {
            SetStatusMessage("Name may only use letters, numbers, underscore.");
            return;
        }
    }

    VfxAsset asset{};
    asset.name = name;
    asset.emitter.CaptureFrom(*previewEmitter);
    asset.builtinName.Clear();

    Utf8String dir(SPARK_BUILD_ASSETS_DIR);
    dir.AppendUtf8("/vfx/custom");
    mkdir(dir.CStr(), 0755);
    Utf8String path = dir;
    path.AppendUtf8("/");
    path.AppendUtf8(name.CStr());
    path.AppendUtf8(".sparkvfx");

    if (!VfxAssetLoader::TrySaveToFile(path.CStr(), asset, SPARK_BUILD_ASSETS_DIR)) {
        SetStatusMessage("Failed to save .sparkvfx (check assets path).");
        return;
    }
    SetStatusMessage(std::format("Saved vfx/custom/{}.sparkvfx", name.CStr()).c_str());
}

void VfxShowcaseDemo::Load(GameWorld& w, IEngineContext& context) {
    roots.Clear();
    groundAsset = MakeShared<Mesh>(Utf8String("VfxShowcaseGround"));
    *groundAsset = Mesh::CreateGroundPlane(kSceneGroundHalfExtent);
    groundObject = w.CreateGameObject();
    groundObject->GetName() = Utf8String("Ground");
    groundObject->AddComponent<TransformComponent>();
    groundObject->AddComponent<MeshComponent>(groundAsset, SceneMeshSlot::GroundPlane, Vector3{0.32F, 0.38F, 0.34F});
    roots.PushBack(groundObject);

    pedestalAsset = MakeShared<Mesh>(Utf8String("VfxShowcasePedestal"));
    *pedestalAsset = Mesh::CreateUnitCube();
    pedestalObject = w.CreateGameObject();
    pedestalObject->GetName() = Utf8String("Pedestal");
    if (TransformComponent* tr = pedestalObject->AddComponent<TransformComponent>()) {
        tr->SetTranslation({previewPosition.x, 0.18F, previewPosition.z});
        tr->SetScale({1.2F, 0.36F, 1.2F});
    }
    pedestalObject->AddComponent<MeshComponent>(
            pedestalAsset, SceneMeshSlot::UnitCube, Vector3{0.42F, 0.44F, 0.48F});
    roots.PushBack(pedestalObject);

    previewObject = w.CreateGameObject();
    previewObject->GetName() = Utf8String("VfxPreview");
    if (TransformComponent* tr = previewObject->AddComponent<TransformComponent>()) {
        tr->SetTranslation(previewPosition);
    }
    previewEmitter = previewObject->AddComponent<ParticleEmitterComponent>();
    roots.PushBack(previewObject);

    helpHud.Mount(w, "VFX Showcase");
    helpHud.SetDetail("Built-in library · tune · save custom effects");
    helpHud.SetControlHints(
            "Space play · RMB orbit · Scroll zoom · MMB pan · F1 fly · Home reset camera");

    guiSelectedEffect = 0;
    ApplySelectedPresetToPreview();
    BuildRetainedUi(w);
    SyncTuningFromEmitter();

    context.GetInput().SetCursorCaptured(false);
    ResetCamera();
}

void VfxShowcaseDemo::Unload(GameWorld& w) {
    helpHud.Unmount(w);
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    groundObject = nullptr;
    pedestalObject = nullptr;
    previewObject = nullptr;
    previewEmitter = nullptr;
    uiCanvas = nullptr;
    uiRoot = nullptr;
    uiEffectList = nullptr;
    uiSliderEmission = nullptr;
    uiSliderLifeMin = nullptr;
    uiSliderLifeMax = nullptr;
    uiSliderSizeStart = nullptr;
    uiSliderSizeEnd = nullptr;
    uiSliderSpread = nullptr;
    uiSliderSpeedMin = nullptr;
    uiSliderSpeedMax = nullptr;
    uiSliderGravY = nullptr;
    uiSliderRingRadius = nullptr;
    uiSliderBurstCount = nullptr;
    uiSliderColorStartR = nullptr;
    uiSliderColorStartG = nullptr;
    uiSliderColorStartB = nullptr;
    uiSliderColorStartA = nullptr;
    uiSliderColorEndR = nullptr;
    uiSliderColorEndG = nullptr;
    uiSliderColorEndB = nullptr;
    uiSliderColorEndA = nullptr;
    uiCheckboxEnabled = nullptr;
    uiCheckboxLocalEmission = nullptr;
    uiSaveNameBox = nullptr;
    uiStatusLabel = nullptr;
}

void VfxShowcaseDemo::PlaySelectedEffect(GameWorld& world) {
    const VfxShowcaseDetail::Entry& entry = SelectedEntry();
    if (entry.isComposite) {
        world.GetVfxSubsystem().Queue(entry.assetKey, previewPosition);
        ProcessVfx(world);
        SetStatusMessage(std::format("Played composite '{}'.", entry.assetKey).c_str());
        return;
    }
    if (entry.isBurst) {
        world.GetVfxSubsystem().Queue(entry.assetKey, previewPosition);
        ProcessVfx(world);
        SetStatusMessage(std::format("Played burst '{}'.", entry.assetKey).c_str());
        return;
    }
    if (previewEmitter != nullptr && previewObject != nullptr) {
        const std::uint32_t burst = static_cast<std::uint32_t>(guiBurstCount + 0.5F);
        if (burst > 0) {
            previewEmitter->Burst(*previewObject, burst);
            SetStatusMessage(std::format("Burst {} particles.", burst).c_str());
        }
    }
}

void VfxShowcaseDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world) {
    activeWorld = &world;
    ApplyTuningToEmitter();
    ProcessVfx(world);

    IInput& in = context.GetInput();
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        in.SetCursorCaptured(!in.IsCursorCaptured());
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_HOME)) {
        ResetCamera();
    }

    const bool uiConsumesPointer = UiConsumesGamePointer();
    const bool inViewport = !uiConsumesPointer;
    FlyCamera& cam = cameraController.camera;
    if (in.IsCursorCaptured()) {
        cameraController.UpdateFlyNavigation(in, timing);
    } else if (!uiConsumesPointer) {
        if (in.IsMouseButtonPressedThisFrame(1) && inViewport) {
            cameraController.BeginOrbitDrag(previewPosition);
        }
        if (cameraController.IsOrbitDragging() && in.IsMouseButtonDown(1) && timing.frameIndex > 0) {
            OrbitCameraAroundPivot(
                    cam,
                    cameraController.orbitPivot,
                    cameraController.orbitDistance,
                    in.GetMouseDeltaX(),
                    in.GetMouseDeltaY());
        }
        if (in.IsMouseButtonReleasedThisFrame(1)) {
            cameraController.EndOrbitDrag();
        }
        if (in.IsMouseButtonDown(2) && inViewport && timing.frameIndex > 0) {
            const float panScale = 0.014F * std::max(1.0F, cameraController.orbitDistance * 0.08F);
            PanCamera(cam, in.GetMouseDeltaX(), in.GetMouseDeltaY(), panScale);
        }
        if (inViewport && std::fabs(in.GetScrollDeltaY()) > 1.0e-4F && !UiScrollWheelConsumed()) {
            cam.position += cam.Forward() * (in.GetScrollDeltaY() * 0.65F);
            const Vector3 offset{
                    cam.position.x - previewPosition.x,
                    cam.position.y - previewPosition.y,
                    cam.position.z - previewPosition.z};
            cameraController.orbitDistance = std::max(1.5F, offset.Length());
        }
        if (inViewport || in.IsMouseButtonDown(1) || in.IsMouseButtonDown(2)) {
            cam.ProcessMovement(in, timing.deltaTimeSeconds);
        }
    }

    if (in.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
        PlaySelectedEffect(world);
    }

    helpHud.SetDetail(
            std::format(
                    "{} — Space play · RMB orbit · Scroll zoom · F1: {}",
                    SelectedEntry().label,
                    in.IsCursorCaptured() ? "release fly" : "fly camera")
                    .c_str());
    helpHud.Update(timing, context);
}

void VfxShowcaseDemo::Render(Scene& scene, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.12F, 400.0F);
    const Matrix4 view = cameraController.camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    SceneRenderParams params{};
    params.viewProjection = viewProj;
    params.cameraPositionWorld = cameraController.camera.position;
    params.lightDirectionWorld = Vector3{0.4F, 0.85F, 0.2F}.Normalized();
    params.lightColor = {1.0F, 0.98F, 0.92F};
    params.lightIntensity = 0.92F;
    params.ambientColor = {0.10F, 0.11F, 0.14F};

    const Vector3 f = cameraController.camera.Forward();
    Vector3 r = Vector3::Cross(Vector3::UnitY, f);
    if (r.LengthSquared() < 1.0e-10F) {
        r = Vector3::UnitX;
    } else {
        r = r.Normalized();
    }
    const Vector3 u = Vector3::Cross(f, r).Normalized();
    params.particleCameraRight = r;
    params.particleCameraUp = u;

    params.particles.Clear();
    scene.ForEachParticleEmitter([&params](const ParticleEmitterComponent& pe, const Matrix4& /*world*/) {
        Array<SceneParticleInstance> chunk;
        pe.CollectInstances(chunk);
        for (std::size_t ci = 0; ci < chunk.GetSize(); ++ci) {
            if (params.particles.GetSize() >= SceneRenderParams::MaxParticles) {
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

    scene.ForEachDrawable([&](GameObject* obj, const MeshComponent& mc, const MaterialComponent* mat,
                                 const Matrix4& worldMatrix) {
        if (obj != nullptr && obj->GetComponent<SkyComponent>() != nullptr) {
            return;
        }
        SceneDrawItem item{};
        item.model = worldMatrix;
        item.mesh = mc.GetSlot();
        if (mc.GetSlot() == SceneMeshSlot::Custom) {
            item.customMesh = mc.GetMesh();
        }
        Vector3 alb = mc.GetAlbedo();
        item.textureLayer = -1;
        if (mat != nullptr) {
            ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
        }
        item.albedo = alb;
        params.draws.PushBack(item);
    });

    scene.ForEachTextOverlay([&params](const TextOverlayComponent& tc) {
        ScreenTextDraw d{};
        d.text = tc.GetText();
        d.x = tc.GetScreenX();
        d.y = tc.GetScreenY();
        d.sizePixels = tc.GetFontSizePixels();
        d.color = tc.GetColor();
        d.alpha = tc.GetAlpha();
        d.paintOrder = params.NextUiPaintOrder();
        params.screenTexts.PushBack(MoveTemp(d));
    });

    PaintUiCanvases(world, params, fbW, fbH);
    helpHud.PatchSceneRenderParams(params, world);
    context.SetSceneRenderParams(params);
}

ParticleEmitterComponent* VfxShowcaseDemo::PreviewEmitter() noexcept {
    return previewEmitter;
}

void VfxShowcaseDemo::ApplySelectedPresetToPreview() noexcept {
    if (previewEmitter == nullptr) {
        return;
    }
    VfxShowcaseDetail::ApplyEntryToEmitter(guiSelectedEffect, *previewEmitter);
    guiBurstCount = static_cast<float>(VfxShowcaseDetail::BurstCountForEntry(guiSelectedEffect));
    if (SelectedEntry().isComposite) {
        guiEnabled = false;
    }
}

void VfxShowcaseDemo::SyncTuningFromEmitter() noexcept {
    ParticleEmitterComponent* pe = PreviewEmitter();
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
    guiRingRadius = pe->GetRingRadius();
    guiEnabled = pe->IsEmitterEnabled();
    guiLocalEmission = pe->GetUseLocalEmission();
    guiColorStart = pe->GetColorStart();
    guiColorEnd = pe->GetColorEnd();
    guiEmissionModule = ModuleIndexFromId(pe->GetEmissionModuleId());
    guiModuleSlider = static_cast<float>(guiEmissionModule);

    if (uiEffectList != nullptr) {
        uiEffectList->SetSelectedIndex(guiSelectedEffect);
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
    if (uiSliderRingRadius != nullptr) {
        uiSliderRingRadius->SetValue(guiRingRadius);
    }
    if (uiSliderBurstCount != nullptr) {
        uiSliderBurstCount->SetValue(guiBurstCount);
    }
    if (uiSliderColorStartR != nullptr) {
        uiSliderColorStartR->SetValue(guiColorStart.x);
    }
    if (uiSliderColorStartG != nullptr) {
        uiSliderColorStartG->SetValue(guiColorStart.y);
    }
    if (uiSliderColorStartB != nullptr) {
        uiSliderColorStartB->SetValue(guiColorStart.z);
    }
    if (uiSliderColorStartA != nullptr) {
        uiSliderColorStartA->SetValue(guiColorStart.w);
    }
    if (uiSliderColorEndR != nullptr) {
        uiSliderColorEndR->SetValue(guiColorEnd.x);
    }
    if (uiSliderColorEndG != nullptr) {
        uiSliderColorEndG->SetValue(guiColorEnd.y);
    }
    if (uiSliderColorEndB != nullptr) {
        uiSliderColorEndB->SetValue(guiColorEnd.z);
    }
    if (uiSliderColorEndA != nullptr) {
        uiSliderColorEndA->SetValue(guiColorEnd.w);
    }
    if (uiCheckboxEnabled != nullptr) {
        uiCheckboxEnabled->SetValue(guiEnabled);
    }
    if (uiCheckboxLocalEmission != nullptr) {
        uiCheckboxLocalEmission->SetValue(guiLocalEmission);
    }
}

void VfxShowcaseDemo::ApplyTuningToEmitter() noexcept {
    ParticleEmitterComponent* pe = PreviewEmitter();
    if (pe == nullptr || SelectedEntry().isComposite) {
        return;
    }
    pe->SetEmissionRate(guiEmission);
    pe->SetLifetime(guiLifeMin, std::max(guiLifeMin, guiLifeMax));
    pe->SetStartEndSize(guiSizeStart, guiSizeEnd);
    pe->SetSpreadAngleRadians(guiSpread);
    pe->SetSpeedRange(guiSpeedMin, std::max(guiSpeedMin, guiSpeedMax));
    const Vector3 g = pe->GetGravity();
    pe->SetGravity({g.x, guiGravY, g.z});
    pe->SetRingRadius(guiRingRadius);
    pe->SetEmitterEnabled(guiEnabled);
    pe->SetUseLocalEmission(guiLocalEmission);
    pe->SetEmissionModuleId(ModuleIdFromIndex(guiEmissionModule));
    pe->SetStartEndColor(guiColorStart, guiColorEnd);
}

void VfxShowcaseDemo::BuildRetainedUi(GameWorld& world) {
    if (uiRoot != nullptr) {
        world.DestroyGameObject(uiRoot);
        uiRoot = nullptr;
        uiCanvas = nullptr;
    }
    uiRoot = world.CreateGameObject();
    uiRoot->GetName() = Utf8String("VfxShowcaseUi");
    uiCanvas = uiRoot->AddComponent<UiCanvasComponent>();
    uiCanvas->SetSortOrder(100);
    uiCanvas->SetTheme(Ui::UiTheme::ClassicMint());
    roots.PushBack(uiRoot);

    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    Ui::PanelDesc panelDesc{};
    panelDesc.id = Utf8String("vfx_showcase");
    panelDesc.title = Utf8String("VFX showcase");
    panelDesc.width = DemoGui::kDemoSidePanelWidth;
    panelDesc.height = 760.0F;
    panelDesc.anchorRight = true;
    panelDesc.movable = true;
    panelDesc.edgeMargin = 12.0F;
    auto panel = factory.CreatePanel(panelDesc);

    Ui::LabelDesc browseHdr{};
    browseHdr.id = Utf8String("browse_hdr");
    browseHdr.text = Utf8String("Built-in library");
    AdoptUiChild(*panel, factory.CreateLabel(browseHdr));

    Ui::ListDesc listDesc{};
    listDesc.id = Utf8String("fx_list");
    listDesc.rowHeight = 26.0F;
    listDesc.verticalScrollingEnabled = true;
    auto list = factory.CreateList(listDesc);
    uiEffectList = list.Get();
    Array<Utf8String> items;
    items.Reserve(static_cast<std::size_t>(VfxShowcaseDetail::kEntryCount));
    for (int i = 0; i < VfxShowcaseDetail::kEntryCount; ++i) {
        items.PushBack(Utf8String(VfxShowcaseDetail::kCatalog[static_cast<std::size_t>(i)].label));
    }
    list->SetItems(MoveTemp(items));
    list->SetSelectedIndex(guiSelectedEffect);
    static VfxShowcaseBinding listBinding{};
    listBinding.demo = this;
    Ui::UiIntCallback selectCb{};
    selectCb.fn = &EffectListSelected;
    selectCb.userData = &listBinding;
    list->SetOnSelectionChanged(selectCb);
    AdoptUiChild(*panel, MoveTemp(list));

    Ui::ButtonDesc playDesc{};
    playDesc.id = Utf8String("play");
    playDesc.label = Utf8String("Play effect (Space)");
    auto playBtn = factory.CreateButton(playDesc);
    static VfxShowcaseBinding playBinding{};
    playBinding.demo = this;
    Ui::UiVoidCallback playCb{};
    playCb.fn = &PlayEffectClicked;
    playCb.userData = &playBinding;
    playBtn->SetOnClick(playCb);
    AdoptUiChild(*panel, MoveTemp(playBtn));

    Ui::SeparatorDesc sepDesc{};
    sepDesc.id = Utf8String("sep_tune");
    AdoptUiChild(*panel, factory.CreateSeparator(sepDesc));

    Ui::LabelDesc tuneHdr{};
    tuneHdr.id = Utf8String("tune_hdr");
    tuneHdr.text = Utf8String("Preview emitter tuning");
    AdoptUiChild(*panel, factory.CreateLabel(tuneHdr));

    Ui::ScrollPanelDesc scrollDesc{};
    scrollDesc.id = Utf8String("vfx_scroll");
    scrollDesc.fillRemainingHeight = true;
    auto scrollPanel = factory.CreateScrollPanel(scrollDesc);

    AddSlider(*scrollPanel, factory, "emission", "Emission / sec", &guiEmission, &uiSliderEmission, 0.0F, 360.0F);
    AddSlider(*scrollPanel, factory, "lmin", "Lifetime min (s)", &guiLifeMin, &uiSliderLifeMin, 0.02F, 4.0F);
    AddSlider(*scrollPanel, factory, "lmax", "Lifetime max (s)", &guiLifeMax, &uiSliderLifeMax, 0.05F, 5.0F);
    AddSlider(*scrollPanel, factory, "sz0", "Size start", &guiSizeStart, &uiSliderSizeStart, 0.01F, 0.6F);
    AddSlider(*scrollPanel, factory, "sz1", "Size end", &guiSizeEnd, &uiSliderSizeEnd, 0.005F, 0.6F);

    Ui::LabelDesc colorHdr{};
    colorHdr.id = Utf8String("color_hdr");
    colorHdr.text = Utf8String("Color start");
    colorHdr.muted = true;
    AdoptUiChild(*scrollPanel, factory.CreateLabel(colorHdr));
    AddSlider(*scrollPanel, factory, "csr", "Start R", &guiColorStart.x, &uiSliderColorStartR, 0.0F, 1.0F);
    AddSlider(*scrollPanel, factory, "csg", "Start G", &guiColorStart.y, &uiSliderColorStartG, 0.0F, 1.0F);
    AddSlider(*scrollPanel, factory, "csb", "Start B", &guiColorStart.z, &uiSliderColorStartB, 0.0F, 1.0F);
    AddSlider(*scrollPanel, factory, "csa", "Start A", &guiColorStart.w, &uiSliderColorStartA, 0.0F, 1.0F);

    Ui::LabelDesc colorEndHdr{};
    colorEndHdr.id = Utf8String("color_end_hdr");
    colorEndHdr.text = Utf8String("Color end");
    colorEndHdr.muted = true;
    AdoptUiChild(*scrollPanel, factory.CreateLabel(colorEndHdr));
    AddSlider(*scrollPanel, factory, "cer", "End R", &guiColorEnd.x, &uiSliderColorEndR, 0.0F, 1.0F);
    AddSlider(*scrollPanel, factory, "ceg", "End G", &guiColorEnd.y, &uiSliderColorEndG, 0.0F, 1.0F);
    AddSlider(*scrollPanel, factory, "ceb", "End B", &guiColorEnd.z, &uiSliderColorEndB, 0.0F, 1.0F);
    AddSlider(*scrollPanel, factory, "cea", "End A", &guiColorEnd.w, &uiSliderColorEndA, 0.0F, 1.0F);

    AddSlider(*scrollPanel, factory, "spread", "Spread (rad)", &guiSpread, &uiSliderSpread, 0.0F, 3.14159F);
    AddSlider(*scrollPanel, factory, "spmin", "Speed min", &guiSpeedMin, &uiSliderSpeedMin, 0.0F, 12.0F);
    AddSlider(*scrollPanel, factory, "spmax", "Speed max", &guiSpeedMax, &uiSliderSpeedMax, 0.0F, 14.0F);
    AddSlider(*scrollPanel, factory, "grav", "Gravity Y", &guiGravY, &uiSliderGravY, -12.0F, 12.0F);
    AddSlider(*scrollPanel, factory, "ring", "Ring radius", &guiRingRadius, &uiSliderRingRadius, 0.0F, 2.0F);
    AddSlider(
            *scrollPanel,
            factory,
            "burst",
            "Manual burst count",
            &guiBurstCount,
            &uiSliderBurstCount,
            0.0F,
            256.0F);

    Ui::SliderDesc moduleDesc{};
    moduleDesc.id = Utf8String("module");
    moduleDesc.label = Utf8String("Emission module (0=cont 1=burst 2=ring)");
    moduleDesc.value = guiModuleSlider;
    moduleDesc.minValue = 0.0F;
    moduleDesc.maxValue = 2.0F;
    auto moduleSlider = factory.CreateSlider(moduleDesc);
    static VfxShowcaseBinding moduleBinding{};
    moduleBinding.demo = this;
    Ui::UiFloatCallback moduleCb{};
    moduleCb.fn = &ModuleSliderChanged;
    moduleCb.userData = this;
    moduleSlider->SetOnChanged(moduleCb);
    AdoptUiChild(*scrollPanel, MoveTemp(moduleSlider));

    AdoptUiChild(*panel, MoveTemp(scrollPanel));

    Ui::CheckBoxDesc enabledDesc{};
    enabledDesc.id = Utf8String("enabled");
    enabledDesc.label = Utf8String("Emitter enabled");
    enabledDesc.value = guiEnabled;
    auto enabledBox = factory.CreateCheckBox(enabledDesc);
    uiCheckboxEnabled = enabledBox.Get();
    enabledBox->SetOnChanged(MakeBoolBinding(&guiEnabled));
    AdoptUiChild(*panel, MoveTemp(enabledBox));

    Ui::CheckBoxDesc localDesc{};
    localDesc.id = Utf8String("local");
    localDesc.label = Utf8String("Local emission");
    localDesc.value = guiLocalEmission;
    auto localBox = factory.CreateCheckBox(localDesc);
    uiCheckboxLocalEmission = localBox.Get();
    localBox->SetOnChanged(MakeBoolBinding(&guiLocalEmission));
    AdoptUiChild(*panel, MoveTemp(localBox));

    Ui::ButtonDesc resetDesc{};
    resetDesc.id = Utf8String("reset");
    resetDesc.label = Utf8String("Reset to built-in preset");
    auto resetBtn = factory.CreateButton(resetDesc);
    static VfxShowcaseBinding resetBinding{};
    resetBinding.demo = this;
    Ui::UiVoidCallback resetCb{};
    resetCb.fn = &ResetPresetClicked;
    resetCb.userData = &resetBinding;
    resetBtn->SetOnClick(resetCb);
    AdoptUiChild(*panel, MoveTemp(resetBtn));

    Ui::SeparatorDesc sepSave{};
    sepSave.id = Utf8String("sep_save");
    AdoptUiChild(*panel, factory.CreateSeparator(sepSave));

    Ui::LabelDesc saveHdr{};
    saveHdr.id = Utf8String("save_hdr");
    saveHdr.text = Utf8String("Create custom .sparkvfx");
    AdoptUiChild(*panel, factory.CreateLabel(saveHdr));

    Ui::TextFieldDesc nameDesc{};
    nameDesc.id = Utf8String("save_name");
    nameDesc.label = Utf8String("Effect name");
    nameDesc.text = guiSaveName;
    auto nameBox = factory.CreateTextBox(nameDesc);
    uiSaveNameBox = nameBox.Get();
    static VfxShowcaseBinding nameBinding{};
    nameBinding.demo = this;
    Ui::UiVoidCallback nameCommitCb{};
    nameCommitCb.fn = &SaveNameCommitted;
    nameCommitCb.userData = this;
    nameBox->SetOnCommit(nameCommitCb);
    AdoptUiChild(*panel, MoveTemp(nameBox));

    Ui::ButtonDesc saveDesc{};
    saveDesc.id = Utf8String("save");
    saveDesc.label = Utf8String("Save tuned effect");
    auto saveBtn = factory.CreateButton(saveDesc);
    static VfxShowcaseBinding saveBinding{};
    saveBinding.demo = this;
    Ui::UiVoidCallback saveCb{};
    saveCb.fn = &SaveEffectClicked;
    saveCb.userData = &saveBinding;
    saveBtn->SetOnClick(saveCb);
    AdoptUiChild(*panel, MoveTemp(saveBtn));

    Ui::LabelDesc statusDesc{};
    statusDesc.id = Utf8String("status");
    statusDesc.text = Utf8String("Select an effect and press Space to play.");
    statusDesc.muted = true;
    auto statusLabel = factory.CreateLabel(statusDesc);
    uiStatusLabel = statusLabel.Get();
    AdoptUiChild(*panel, MoveTemp(statusLabel));

    uiCanvas->SetRoot(MoveTemp(panel));
}

}  // namespace Spark
