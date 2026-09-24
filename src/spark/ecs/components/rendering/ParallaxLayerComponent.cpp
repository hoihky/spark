#include "spark/ecs/components/rendering/ParallaxLayerComponent.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"

#include <cmath>

namespace Spark {

void ParallaxLayerComponent::OnAttach(GameObject& owner) {
    if (restCaptured) {
        return;
    }
    if (const TransformComponent* tr = owner.GetComponent<TransformComponent>()) {
        restOffset = tr->GetLocalTransform().translation;
        restCaptured = true;
    }
}

void ParallaxLayerComponent::Tick(
        ParallaxLayerComponent& layer,
        GameObject& owner,
        const float deltaSeconds,
        const float sceneTime) noexcept {
    TransformComponent* tr = owner.GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }

    layer.driftTime = sceneTime;
    float cameraX = layer.anchorWorld.x;
    float cameraY = layer.anchorWorld.y;
    if (layer.cameraReference != nullptr) {
        const Vector3 camPos = layer.cameraReference->GetWorldMatrix().TranslationVector();
        cameraX = camPos.x;
        cameraY = camPos.y;
    }

    const float dx = cameraX - layer.anchorWorld.x;
    const float dy = cameraY - layer.anchorWorld.y;

    Vector3 pos = layer.restOffset;
    if (layer.axisMode == ParallaxAxisMode::Both) {
        pos.x += dx * layer.factorX;
        pos.y += dy * layer.factorY;
    } else {
        pos.x += dx * layer.factorX;
    }

    if (layer.driftMode == ParallaxDriftMode::SineHorizontal && layer.driftAmplitude > 0.0F) {
        const float w = layer.driftFrequencyHz * 6.2831853F;
        pos.x += std::sin(layer.driftTime * w + layer.driftPhase) * layer.driftAmplitude;
    }

    (void)deltaSeconds;
    tr->SetTranslation(pos);
}

void ParallaxLayerComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    if (!restCaptured) {
        OnAttach(owner);
    }
    Tick(*this, owner, timing.deltaTimeSeconds, driftTime + timing.deltaTimeSeconds);
}

}  // namespace Spark
