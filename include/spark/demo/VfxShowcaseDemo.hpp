#pragma once

#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/VfxShowcaseDetail.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/demo/DemoGuiFrame.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/editor/SceneEditorCameraController.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/ui/runtime/UiScene.hpp"
#include "spark/ui/runtime/UiSystem.hpp"
#include "spark/ui/spark/SparkUiControlsFactory.hpp"
#include "spark/ui/spark/UiChild.hpp"

namespace Spark {

class IEngineContext;
struct SceneRenderParams;

class VfxShowcaseDemo {
public:
    void Load(GameWorld& world, IEngineContext& context);
    void Unload(GameWorld& world);
    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

    void OnEffectListSelected(int index);
    void OnResetPresetClicked();
    void OnPlayEffectClicked();
    void OnSaveEffectClicked();
    void OnModuleSliderChanged(float value);
    void OnSaveNameCommitted();

private:
    void BuildRetainedUi(GameWorld& world);
    void SyncTuningFromEmitter() noexcept;
    void ApplyTuningToEmitter() noexcept;
    void ApplySelectedPresetToPreview() noexcept;
    void PlaySelectedEffect(GameWorld& world);
    [[nodiscard]] ParticleEmitterComponent* PreviewEmitter() noexcept;
    [[nodiscard]] const VfxShowcaseDetail::Entry& SelectedEntry() const noexcept;
    void SetStatusMessage(const char* message);
    void ResetCamera() noexcept;

    Array<GameObject*> roots{};
    GameObject* groundObject = nullptr;
    GameObject* pedestalObject = nullptr;
    GameObject* previewObject = nullptr;
    SharedPtr<Mesh> groundAsset;
    SharedPtr<Mesh> pedestalAsset;
    ParticleEmitterComponent* previewEmitter = nullptr;
    GameWorld* activeWorld = nullptr;
    DemoHelpHud helpHud{};
    SceneEditorCameraController cameraController{};
    GameObject* uiRoot = nullptr;
    UiCanvasComponent* uiCanvas = nullptr;
    Ui::IList* uiEffectList = nullptr;
    Ui::ISlider* uiSliderEmission = nullptr;
    Ui::ISlider* uiSliderLifeMin = nullptr;
    Ui::ISlider* uiSliderLifeMax = nullptr;
    Ui::ISlider* uiSliderSizeStart = nullptr;
    Ui::ISlider* uiSliderSizeEnd = nullptr;
    Ui::ISlider* uiSliderSpread = nullptr;
    Ui::ISlider* uiSliderSpeedMin = nullptr;
    Ui::ISlider* uiSliderSpeedMax = nullptr;
    Ui::ISlider* uiSliderGravY = nullptr;
    Ui::ISlider* uiSliderRingRadius = nullptr;
    Ui::ISlider* uiSliderBurstCount = nullptr;
    Ui::ISlider* uiSliderColorStartR = nullptr;
    Ui::ISlider* uiSliderColorStartG = nullptr;
    Ui::ISlider* uiSliderColorStartB = nullptr;
    Ui::ISlider* uiSliderColorStartA = nullptr;
    Ui::ISlider* uiSliderColorEndR = nullptr;
    Ui::ISlider* uiSliderColorEndG = nullptr;
    Ui::ISlider* uiSliderColorEndB = nullptr;
    Ui::ISlider* uiSliderColorEndA = nullptr;
    Ui::ICheckBox* uiCheckboxEnabled = nullptr;
    Ui::ICheckBox* uiCheckboxLocalEmission = nullptr;
    Ui::ITextBox* uiSaveNameBox = nullptr;
    Ui::ILabel* uiStatusLabel = nullptr;
    int guiSelectedEffect = 0;
    int guiEmissionModule = 0;
    float guiModuleSlider = 0.0F;
    float guiEmission = 110.0F;
    float guiLifeMin = 0.22F;
    float guiLifeMax = 0.55F;
    float guiSizeStart = 0.24F;
    float guiSizeEnd = 0.03F;
    float guiSpread = 0.55F;
    float guiSpeedMin = 1.6F;
    float guiSpeedMax = 4.2F;
    float guiGravY = 0.35F;
    float guiRingRadius = 0.35F;
    float guiBurstCount = 48.0F;
    Vector4 guiColorStart{0.95F, 0.85F, 0.35F, 1.0F};
    Vector4 guiColorEnd{0.9F, 0.2F, 0.05F, 0.0F};
    bool guiEnabled = true;
    bool guiLocalEmission = true;
    Utf8String guiSaveName{"my_effect"};
    Utf8String guiStatus{};
    Vector3 previewPosition{0.0F, 0.55F, 0.0F};
};

}  // namespace Spark
