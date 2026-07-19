#pragma once

#include "spark/demo/DemoFoundation.hpp"
#include "spark/ecs/components/camera/Camera2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Texture2D.hpp"

namespace Spark::Platformer2D {

/**
 * Screen-anchored health bar built from world sprites parented to the camera each frame.
 * Observes health values and maps them to a fill width (progress-bar / "blood meter" pattern).
 */
class HealthHud final {
public:
    void Initialize(
            Spark::GameWorld& world,
            const Spark::SharedPtr<Spark::Texture2D>& whitePixelTex,
            Spark::DemoRootCollection& roots);

    void Shutdown(Spark::GameWorld& world) noexcept;

    void SyncToCamera(
            const Spark::Camera2DComponent& camera,
            const Spark::GameObject& cameraOwner,
            float framebufferWidth,
            float framebufferHeight) noexcept;

    void SetHealth(float current, float maximum) noexcept;

private:
    Spark::GameObject* root = nullptr;
    Spark::TransformComponent* rootTr = nullptr;
    Spark::TransformComponent* trackTr = nullptr;
    Spark::TransformComponent* fillTr = nullptr;
    Spark::TransformComponent* glossTr = nullptr;
    Spark::SpriteComponent* fillSpr = nullptr;
    Spark::TextOverlayComponent* healthText = nullptr;

    float currentHealth = 0.0F;
    float maxHealth = 1.0F;
    float barWidthWorld = 8.0F;
    float barHeightWorld = 0.42F;
    float displayedRatio = 1.0F;
    float targetRatio = 1.0F;
};

}  // namespace Spark::Platformer2D
