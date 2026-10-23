#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Cone spotlight at the object's world position; forward axis is local **-Z** transformed by the world matrix.
 * Collected into <c>SceneRenderParams::spotLights</c> (see <c>FillStandardLitSceneFromWorld</c>).
 */
class SpotLightComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpotLight;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SpotLightComponent() = default;
    SpotLightComponent(Vector3 inColor, float inIntensity, float inRange, float innerConeDeg, float outerConeDeg);

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

    SPARK_SCRIPT_BIND(get_color)
    [[nodiscard]] const Vector3& GetColor() const noexcept { return color; }
    SPARK_SCRIPT_BIND(get_intensity)
    [[nodiscard]] float GetIntensity() const noexcept { return intensity; }
    SPARK_SCRIPT_BIND(get_range)
    [[nodiscard]] float GetRange() const noexcept { return range; }
    /** Full cone angle in degrees (must be &lt;= outer). */
    SPARK_SCRIPT_BIND(get_inner_cone_degrees)
    [[nodiscard]] float GetInnerConeDegrees() const noexcept { return innerConeDegrees; }
    /** Full cone angle in degrees (must be &gt;= inner). */
    SPARK_SCRIPT_BIND(get_outer_cone_degrees)
    [[nodiscard]] float GetOuterConeDegrees() const noexcept { return outerConeDegrees; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(is_casts_shadow)
    [[nodiscard]] bool CastsShadow() const noexcept { return castsShadow; }

    void SetColor(const Vector3& c);
    void SetIntensity(float v);
    void SetRange(float r);
    SPARK_SCRIPT_BIND(set_inner_cone_degrees)
    void SetInnerConeDegrees(float deg);
    SPARK_SCRIPT_BIND(set_outer_cone_degrees)
    void SetOuterConeDegrees(float deg);
    void SetEnabled(bool e);
    SPARK_SCRIPT_BIND(set_casts_shadow)
    void SetCastsShadow(bool c);

private:
    Vector3 color{1.0F, 0.95F, 0.88F};
    float intensity = 4.0F;
    float range = 18.0F;
    float innerConeDegrees = 28.0F;
    float outerConeDegrees = 45.0F;
    bool enabled = true;
    bool castsShadow = false;
};

}  // namespace Spark
