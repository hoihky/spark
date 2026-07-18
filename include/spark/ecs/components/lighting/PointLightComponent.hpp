#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * Omnidirectional light at the entity's world position (from GameObject::GetWorldMatrix translation).
 * Collected each frame into SceneRenderParams::pointLights for deferred-style forward lighting on GPU.
 */
class PointLightComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PointLight;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    PointLightComponent() = default;
    PointLightComponent(Vector3 inColor, float inIntensity, float inRange);

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

    [[nodiscard]] const Vector3& GetColor() const noexcept { return color; }
    [[nodiscard]] float GetIntensity() const noexcept { return intensity; }
    [[nodiscard]] float GetRange() const noexcept { return range; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    [[nodiscard]] bool CastsShadow() const noexcept { return castsShadow; }

    void SetColor(const Vector3& c);
    void SetIntensity(float v);
    void SetRange(float r);
    void SetEnabled(bool e);
    void SetCastsShadow(bool c);

private:
    Vector3 color{1.0F, 0.92F, 0.82F};
    float intensity = 2.4F;
    /** Distance at which contribution falls off to ~0 (inverse-square with smooth range window). */
    float range = 12.0F;
    bool enabled = true;
    bool castsShadow = false;
};

}  // namespace Spark
