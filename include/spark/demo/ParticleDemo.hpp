#pragma once

#include "spark/demo/ParticleDemoDetail.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/demo/DemoGuiFrame.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/ui/runtime/UiScene.hpp"
#include "spark/ui/runtime/UiSystem.hpp"
#include "spark/ui/spark/SparkUiControlsFactory.hpp"
#include "spark/ui/spark/UiChild.hpp"

namespace Spark {

class IEngineContext;
struct SceneRenderParams;

class ParticleDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Unload(Spark::GameWorld& w);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);
    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);

    void OnEmitterListSelected(int index);
    void OnResetPresetClicked();

private:
    void BuildRetainedUi(Spark::GameWorld& world);
    void SyncTuningFromEmitter() noexcept;
    void ApplyTuningToEmitter() noexcept;
    [[nodiscard]] Spark::ParticleEmitterComponent* SelectedEmitter() noexcept;

    Spark::Array<Spark::GameObject*> roots{};
    Spark::GameObject* groundObject = nullptr;
    Spark::GameObject* cubeObject = nullptr;
    Spark::SharedPtr<Spark::Mesh> groundAsset;
    Spark::SharedPtr<Spark::Mesh> unitCubeAsset;
    Spark::GameObject* effectObjects[Detail::kParticleDemoEffectCount]{};
    Spark::ParticleEmitterComponent* effectEmitters[Detail::kParticleDemoEffectCount]{};
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    Spark::FlyCamera camera{};
    Spark::GameObject* uiRoot = nullptr;
    Spark::UiCanvasComponent* uiCanvas = nullptr;
    Spark::Ui::IList* uiEmitterList = nullptr;
    Spark::Ui::ISlider* uiSliderEmission = nullptr;
    Spark::Ui::ISlider* uiSliderLifeMin = nullptr;
    Spark::Ui::ISlider* uiSliderLifeMax = nullptr;
    Spark::Ui::ISlider* uiSliderSizeStart = nullptr;
    Spark::Ui::ISlider* uiSliderSizeEnd = nullptr;
    Spark::Ui::ISlider* uiSliderSpread = nullptr;
    Spark::Ui::ISlider* uiSliderSpeedMin = nullptr;
    Spark::Ui::ISlider* uiSliderSpeedMax = nullptr;
    Spark::Ui::ISlider* uiSliderGravY = nullptr;
    Spark::Ui::ICheckBox* uiCheckboxEnabled = nullptr;
    int guiSelectedEffect = 0;
    float guiEmission = 110.0F;
    float guiLifeMin = 0.22F;
    float guiLifeMax = 0.55F;
    float guiSizeStart = 0.24F;
    float guiSizeEnd = 0.03F;
    float guiSpread = 0.55F;
    float guiSpeedMin = 1.6F;
    float guiSpeedMax = 4.2F;
    float guiGravY = 0.35F;
    bool guiEnabled = true;
    float fpsSmoothed = 0.0F;
};

}  // namespace Spark
