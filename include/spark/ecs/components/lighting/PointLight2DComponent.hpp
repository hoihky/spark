#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Omnidirectional light for 2D / orthographic scenes. Position = owner transform XY + local offset; Z from
 * <c>worldZ</c>. Collected into <c>SceneRenderParams::pointLights</c> (no shadow cast by default).
 */
class PointLight2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PointLight2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    PointLight2DComponent() = default;
    PointLight2DComponent(Vector3 colorIn, float intensityIn, float rangeIn) noexcept;

    SPARK_SCRIPT_BIND(get_local_offset)
    [[nodiscard]] const Vector2& GetLocalOffset() const noexcept { return localOffset; }
    SPARK_SCRIPT_BIND(get_world_z)
    [[nodiscard]] float GetWorldZ() const noexcept { return worldZ; }
    SPARK_SCRIPT_BIND(get_color)
    [[nodiscard]] const Vector3& GetColor() const noexcept { return color; }
    SPARK_SCRIPT_BIND(get_intensity)
    [[nodiscard]] float GetIntensity() const noexcept { return intensity; }
    SPARK_SCRIPT_BIND(get_effective_intensity)
    [[nodiscard]] float GetEffectiveIntensity() const noexcept { return effectiveIntensity; }
    SPARK_SCRIPT_BIND(get_range)
    [[nodiscard]] float GetRange() const noexcept { return range; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    SPARK_SCRIPT_BIND(set_local_offset)
    void SetLocalOffset(const Vector2& offset) noexcept { localOffset = offset; }
    SPARK_SCRIPT_BIND(set_world_z)
    void SetWorldZ(const float z) noexcept { worldZ = z; }
    SPARK_SCRIPT_BIND(set_color)
    void SetColor(const Vector3& c) noexcept { color = c; }
    SPARK_SCRIPT_BIND(set_intensity)
    void SetIntensity(const float v) noexcept { intensity = v; }
    SPARK_SCRIPT_BIND(set_range)
    void SetRange(const float r) noexcept { range = r; }
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool e) noexcept { enabled = e; }

    SPARK_SCRIPT_BIND(set_flicker)
    void SetFlicker(const bool on, const float amount, const float frequencyHz) noexcept;
    SPARK_SCRIPT_BIND(get_flicker_enabled)
    [[nodiscard]] bool GetFlickerEnabled() const noexcept { return flickerEnabled; }
    SPARK_SCRIPT_BIND(get_flicker_amount)
    [[nodiscard]] float GetFlickerAmount() const noexcept { return flickerAmount; }
    SPARK_SCRIPT_BIND(get_flicker_hz)
    [[nodiscard]] float GetFlickerHz() const noexcept { return flickerHz; }

private:
    Vector2 localOffset{};
    float worldZ = 0.12F;
    Vector3 color{1.0F, 0.9F, 0.75F};
    float intensity = 2.0F;
    float effectiveIntensity = 2.0F;
    float range = 10.0F;
    bool enabled = true;
    bool flickerEnabled = false;
    float flickerAmount = 0.18F;
    float flickerHz = 9.0F;
    float flickerPhase = 0.0F;
};

}  // namespace Spark
