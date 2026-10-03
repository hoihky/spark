#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scene/camera/Camera2D.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

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
    SPARK_SCRIPT_BIND(get_half_extent_y)
    [[nodiscard]] float GetHalfExtentY() const noexcept { return halfExtentY; }
    SPARK_SCRIPT_BIND(get_clip_near_z)
    [[nodiscard]] float GetClipNearZ() const noexcept { return clipNearZ; }
    SPARK_SCRIPT_BIND(get_clip_far_z)
    [[nodiscard]] float GetClipFarZ() const noexcept { return clipFarZ; }
    SPARK_SCRIPT_BIND(get_priority)
    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    SPARK_SCRIPT_BIND(set_half_extent_y)
    void SetHalfExtentY(float h) noexcept { halfExtentY = h; }
    SPARK_SCRIPT_BIND(set_clip_near_z)
    void SetClipNearZ(float z) noexcept { clipNearZ = z; }
    SPARK_SCRIPT_BIND(set_clip_far_z)
    void SetClipFarZ(float z) noexcept { clipFarZ = z; }
    SPARK_SCRIPT_BIND(set_priority)
    void SetPriority(std::int32_t p) noexcept { priority = p; }
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(bool e) noexcept { enabled = e; }

    /** Snapshot of projection + pose suitable for <c>Camera2D</c> math helpers. */
    SPARK_SCRIPT_BIND(build_camera2_d)
    [[nodiscard]] Camera2D BuildCamera2D(const GameObject& owner) const noexcept;

    SPARK_SCRIPT_BIND(view_matrix)
    [[nodiscard]] Matrix4 ViewMatrix(const GameObject& owner) const noexcept;
    SPARK_SCRIPT_BIND(view_projection)
    [[nodiscard]] Matrix4 ViewProjection(const GameObject& owner, float framebufferWidth, float framebufferHeight)
            const noexcept;

    SPARK_SCRIPT_BIND(world_position)
    [[nodiscard]] Vector3 WorldPosition(const GameObject& owner) const noexcept;

    SPARK_SCRIPT_BIND(billboard_basis_world)
    void BillboardBasisWorld(const GameObject& owner, Vector3& outRight, Vector3& outUp) const noexcept;

private:
    float halfExtentY = 5.0F;
    float clipNearZ = -500.0F;
    float clipFarZ = 500.0F;
    std::int32_t priority = 0;
    bool enabled = true;
};

}  // namespace Spark
