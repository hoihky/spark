#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Function.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <algorithm>
#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class GameWorld;
struct FrameTiming;
struct TriggerVolume2DSettings;
class TriggerVolumeWorld2D;

enum class TriggerVolume2DShape : std::uint8_t {
    Box = 0,
    Circle = 1,
};

/**
 * Non-blocking 2D trigger volume. Call <c>SimulateTriggerVolumes2D</c> each frame after movement / physics.
 * Fires <c>SetOnEnter</c> / <c>SetOnStay</c> / <c>SetOnExit</c> and emits
 * <c>SignalId::Physics2DTriggerEnter</c> / <c>Stay</c> / <c>Exit</c> to sibling components.
 */
class TriggerVolume2DComponent final : public GameComponent {
public:
    using ObjectCallback = Function<void(GameObject& other)>;

    static constexpr ComponentKind TypeKind = ComponentKind::TriggerVolume2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit TriggerVolume2DComponent(
            TriggerVolume2DShape shapeIn = TriggerVolume2DShape::Box,
            Vector2 boxHalfExtents = {0.5F, 0.5F},
            Vector2 localOffset = Vector2::Zero) noexcept
            : shape(shapeIn), halfExtents(boxHalfExtents), offset(localOffset) {}

    SPARK_SCRIPT_BIND(get_shape)
    [[nodiscard]] TriggerVolume2DShape GetShape() const noexcept { return shape; }
    SPARK_SCRIPT_BIND(set_shape)
    void SetShape(TriggerVolume2DShape value) noexcept { shape = value; }

    SPARK_SCRIPT_BIND(get_half_extents)
    [[nodiscard]] const Vector2& GetHalfExtents() const noexcept { return halfExtents; }
    SPARK_SCRIPT_BIND(set_half_extents)
    void SetHalfExtents(const Vector2& value) noexcept { halfExtents = value; }

    SPARK_SCRIPT_BIND(get_radius)
    [[nodiscard]] float GetRadius() const noexcept { return radius; }
    SPARK_SCRIPT_BIND(set_radius)
    void SetRadius(const float value) noexcept { radius = std::max(0.01F, value); }

    SPARK_SCRIPT_BIND(get_offset)
    [[nodiscard]] const Vector2& GetOffset() const noexcept { return offset; }
    SPARK_SCRIPT_BIND(set_offset)
    void SetOffset(const Vector2& value) noexcept { offset = value; }

    /** Optional tag filter: when non-empty, only objects with the same tag are detected. */
    SPARK_SCRIPT_BIND(get_filter_tag)
    [[nodiscard]] const char* GetFilterTag() const noexcept { return filterTag; }
    SPARK_SCRIPT_BIND(set_filter_tag)
    void SetFilterTag(const char* tag) noexcept { filterTag = tag; }

    void SetOnEnter(ObjectCallback callback) { onEnter = MoveTemp(callback); }
    void SetOnStay(ObjectCallback callback) { onStay = MoveTemp(callback); }
    void SetOnExit(ObjectCallback callback) { onExit = MoveTemp(callback); }

    SPARK_SCRIPT_BIND(is_overlapping)
    [[nodiscard]] bool IsOverlapping(const GameObject& other) const noexcept;

private:
    friend void SimulateTriggerVolumes2D(GameWorld& world, const FrameTiming& timing, const TriggerVolume2DSettings& settings);
    friend class TriggerVolumeWorld2D;

    void NotifyEnter(GameObject& other);
    void NotifyStay(GameObject& other);
    void NotifyExit(GameObject& other);
    void SetOverlappingIds(Array<std::uint64_t> ids);

    TriggerVolume2DShape shape = TriggerVolume2DShape::Box;
    Vector2 halfExtents{0.5F, 0.5F};
    float radius = 0.5F;
    Vector2 offset{Vector2::Zero};
    const char* filterTag = nullptr;
    ObjectCallback onEnter{};
    ObjectCallback onStay{};
    ObjectCallback onExit{};
    Array<std::uint64_t> overlappingIds{};
};

}  // namespace Spark
