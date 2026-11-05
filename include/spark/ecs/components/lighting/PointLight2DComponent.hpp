#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"

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

    [[nodiscard]] const Vector2& GetLocalOffset() const noexcept { return localOffset; }
    [[nodiscard]] float GetWorldZ() const noexcept { return worldZ; }
    [[nodiscard]] const Vector3& GetColor() const noexcept { return color; }
    [[nodiscard]] float GetIntensity() const noexcept { return intensity; }
    [[nodiscard]] float GetEffectiveIntensity() const noexcept { return effectiveIntensity; }
    [[nodiscard]] float GetRange() const noexcept { return range; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetLocalOffset(const Vector2& offset) noexcept { localOffset = offset; }
    void SetWorldZ(const float z) noexcept { worldZ = z; }
    void SetColor(const Vector3& c) noexcept { color = c; }
    void SetIntensity(const float v) noexcept { intensity = v; }
    void SetRange(const float r) noexcept { range = r; }
    void SetEnabled(const bool e) noexcept { enabled = e; }

    void SetFlicker(const bool on, const float amount, const float frequencyHz) noexcept;
    [[nodiscard]] bool GetFlickerEnabled() const noexcept { return flickerEnabled; }
    [[nodiscard]] float GetFlickerAmount() const noexcept { return flickerAmount; }
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
