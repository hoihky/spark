#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>
#include <functional>

namespace Spark {

class GameObject;
class GameWorld;
struct FrameTiming;
struct TriggerVolume3DSettings;

/** Local shape stored on <c>TriggerVolume3DComponent</c> (mirrors collider conventions). */
enum class TriggerVolume3DShape : std::uint8_t {
    Box = 0,
    Sphere = 1,
    Capsule = 2,
};

/**
 * Non-blocking 3D trigger volume. Call <c>SimulateTriggerVolumes3D</c> each frame after movement / physics.
 * Fires <c>SetOnEnter</c> / <c>SetOnExit</c> when probe bodies begin or end overlap. Also emits
 * <c>SignalId::Physics3DTriggerEnter</c> / <c>Physics3DTriggerExit</c> to sibling components
 * (<c>payload.ptr</c> = other <c>GameObject*</c>, <c>payload.a</c> = other id).
 */
class TriggerVolume3DComponent final : public GameComponent {
public:
    using ObjectCallback = std::function<void(GameObject& other)>;

    static constexpr ComponentKind TypeKind = ComponentKind::TriggerVolume3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit TriggerVolume3DComponent(
            TriggerVolume3DShape shapeIn = TriggerVolume3DShape::Box,
            Vector3 boxHalfExtents = {0.5F, 0.5F, 0.5F},
            Vector3 localOffset = Vector3::Zero) noexcept
            : shape(shapeIn), halfExtents(boxHalfExtents), offset(localOffset) {}

    [[nodiscard]] TriggerVolume3DShape GetShape() const noexcept { return shape; }
    void SetShape(const TriggerVolume3DShape value) noexcept { shape = value; }

    [[nodiscard]] const Vector3& GetHalfExtents() const noexcept { return halfExtents; }
    void SetHalfExtents(const Vector3& value) noexcept { halfExtents = value; }

    [[nodiscard]] float GetRadius() const noexcept { return radius; }
    void SetRadius(const float value) noexcept { radius = std::max(0.01F, value); }

    [[nodiscard]] float GetHeight() const noexcept { return height; }
    void SetHeight(const float value) noexcept { height = std::max(0.01F, value); }

    [[nodiscard]] CapsuleDirection3D GetCapsuleDirection() const noexcept { return capsuleDirection; }
    void SetCapsuleDirection(const CapsuleDirection3D axis) noexcept { capsuleDirection = axis; }

    [[nodiscard]] const Vector3& GetOffset() const noexcept { return offset; }
    void SetOffset(const Vector3& value) noexcept { offset = value; }

    /** Optional tag filter: when non-empty, only objects with the same tag are detected. */
    [[nodiscard]] const char* GetFilterTag() const noexcept { return filterTag; }
    void SetFilterTag(const char* tag) noexcept { filterTag = tag; }

    void SetOnEnter(ObjectCallback callback) { onEnter = std::move(callback); }
    void SetOnExit(ObjectCallback callback) { onExit = std::move(callback); }

    /** True while <c>other</c> overlaps this volume (after the last <c>SimulateTriggerVolumes3D</c> step). */
    [[nodiscard]] bool IsOverlapping(const GameObject& other) const noexcept;

private:
    friend void SimulateTriggerVolumes3D(GameWorld& world, const FrameTiming& timing, const TriggerVolume3DSettings& settings);

    void NotifyEnter(GameObject& other);
    void NotifyExit(GameObject& other);
    void SetOverlappingIds(Array<std::uint64_t> ids);

    TriggerVolume3DShape shape = TriggerVolume3DShape::Box;
    Vector3 halfExtents{0.5F, 0.5F, 0.5F};
    float radius = 0.5F;
    float height = 2.0F;
    CapsuleDirection3D capsuleDirection = CapsuleDirection3D::Y;
    Vector3 offset{Vector3::Zero};
    const char* filterTag = nullptr;
    ObjectCallback onEnter{};
    ObjectCallback onExit{};
    Array<std::uint64_t> overlappingIds{};
};

}  // namespace Spark
