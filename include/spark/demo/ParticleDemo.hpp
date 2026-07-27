#pragma once

#include "spark/demo/ParticleDemoDetail.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/scene/Scene.hpp"

namespace Spark {

class IEngineContext;
struct SceneRenderParams;

class ParticleDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Unload(Spark::GameWorld& w);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);
    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);

private:
    void BuildPortableUi(Spark::IEngineContext& context, SceneRenderParams& params, const Spark::GameWorld& world);
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
    int guiSelectedEffect = 0;
    float fpsSmoothed = 0.0F;
};

}  // namespace Spark
