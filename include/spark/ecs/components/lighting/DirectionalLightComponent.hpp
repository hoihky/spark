#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(get_color)
    [[nodiscard]] const Vector3& GetColor() const noexcept { return color; }
    SPARK_SCRIPT_BIND(get_intensity)
    [[nodiscard]] float GetIntensity() const noexcept { return intensity; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(is_casts_shadow)
    [[nodiscard]] bool CastsShadow() const noexcept { return castsShadow; }

    SPARK_SCRIPT_BIND(set_color)
    void SetColor(const Vector3& c);
    SPARK_SCRIPT_BIND(set_intensity)
    void SetIntensity(float v);
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(bool e);
    SPARK_SCRIPT_BIND(set_casts_shadow)
    void SetCastsShadow(bool c);

private:
    Vector3 color{1.0F, 0.97F, 0.9F};
    float intensity = 0.92F;
    bool enabled = true;
    bool castsShadow = true;
};

}  // namespace Spark
