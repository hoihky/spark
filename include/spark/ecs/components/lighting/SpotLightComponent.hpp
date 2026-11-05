#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

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

    [[nodiscard]] const Vector3& GetColor() const noexcept { return color; }
    [[nodiscard]] float GetIntensity() const noexcept { return intensity; }
    [[nodiscard]] float GetRange() const noexcept { return range; }
    /** Full cone angle in degrees (must be &lt;= outer). */
    [[nodiscard]] float GetInnerConeDegrees() const noexcept { return innerConeDegrees; }
    /** Full cone angle in degrees (must be &gt;= inner). */
    [[nodiscard]] float GetOuterConeDegrees() const noexcept { return outerConeDegrees; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    [[nodiscard]] bool CastsShadow() const noexcept { return castsShadow; }

    void SetColor(const Vector3& c);
    void SetIntensity(float v);
    void SetRange(float r);
    void SetInnerConeDegrees(float deg);
    void SetOuterConeDegrees(float deg);
    void SetEnabled(bool e);
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
