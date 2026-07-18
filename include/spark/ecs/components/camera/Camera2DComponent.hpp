#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scene/Camera2D.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
struct Matrix4;
struct Vector3;

/**
 * Orthographic 2D camera on a <c>GameObject</c> with a <c>TransformComponent</c>.
 * World pose (pan / Z-rotation) comes from the transform; projection fields live here.
 * Higher <c>priority</c> wins when resolving the main camera for 2D rendering.
 */
class Camera2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Camera2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    Camera2DComponent() = default;

    /** Half of the visible vertical span in world units (same as <c>Camera2D::halfExtentY</c>). */
    [[nodiscard]] float GetHalfExtentY() const noexcept { return halfExtentY; }
    [[nodiscard]] float GetClipNearZ() const noexcept { return clipNearZ; }
    [[nodiscard]] float GetClipFarZ() const noexcept { return clipFarZ; }
    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetHalfExtentY(float h) noexcept { halfExtentY = h; }
    void SetClipNearZ(float z) noexcept { clipNearZ = z; }
    void SetClipFarZ(float z) noexcept { clipFarZ = z; }
    void SetPriority(std::int32_t p) noexcept { priority = p; }
    void SetEnabled(bool e) noexcept { enabled = e; }

    /** Snapshot of projection + pose suitable for <c>Camera2D</c> math helpers. */
    [[nodiscard]] Camera2D BuildCamera2D(const GameObject& owner) const noexcept;

    [[nodiscard]] Matrix4 ViewMatrix(const GameObject& owner) const noexcept;
    [[nodiscard]] Matrix4 ViewProjection(const GameObject& owner, float framebufferWidth, float framebufferHeight)
            const noexcept;

    [[nodiscard]] Vector3 WorldPosition(const GameObject& owner) const noexcept;

    void BillboardBasisWorld(const GameObject& owner, Vector3& outRight, Vector3& outUp) const noexcept;

private:
    float halfExtentY = 5.0F;
    float clipNearZ = -500.0F;
    float clipFarZ = 500.0F;
    std::int32_t priority = 0;
    bool enabled = true;
};

}  // namespace Spark
