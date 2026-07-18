#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * Directional sun / moon light driven by the owning transform.
 * World light direction (for N·L) is local <b>+Z</b> transformed by the world matrix — the opposite of the
 * emission axis (local -Z, same convention as <c>SpotLightComponent</c>).
 * When present and enabled, <c>FillStandardLitSceneFromWorld</c> overrides
 * <c>SceneRenderParams::lightDirectionWorld</c>, <c>lightColor</c>, <c>lightIntensity</c>, and
 * <c>directionalShadowsEnabled</c>.
 */
class DirectionalLightComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::DirectionalLight;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    DirectionalLightComponent() = default;
    DirectionalLightComponent(Vector3 inColor, float inIntensity);

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

    [[nodiscard]] const Vector3& GetColor() const noexcept { return color; }
    [[nodiscard]] float GetIntensity() const noexcept { return intensity; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    [[nodiscard]] bool CastsShadow() const noexcept { return castsShadow; }

    void SetColor(const Vector3& c);
    void SetIntensity(float v);
    void SetEnabled(bool e);
    void SetCastsShadow(bool c);

private:
    Vector3 color{1.0F, 0.97F, 0.9F};
    float intensity = 0.92F;
    bool enabled = true;
    bool castsShadow = true;
};

}  // namespace Spark
