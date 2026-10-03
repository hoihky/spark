#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
struct Matrix4;
struct Vector3;

/** Perspective or orthographic projection for a camera on a GameObject with a TransformComponent. */
enum class CameraProjectionMode : std::uint8_t {
    Perspective = 0,
    Orthographic = 1,
};

/**
 * ECS camera: world pose comes from the owner's transform; projection fields live on this component.
 * Higher <c>priority</c> wins when resolving the main camera for rendering.
 */
class CameraComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Camera;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    CameraComponent() = default;

    SPARK_SCRIPT_BIND(get_projection_mode)
    [[nodiscard]] CameraProjectionMode GetProjectionMode() const noexcept { return projectionMode; }
    SPARK_SCRIPT_BIND(get_fov_y_degrees)
    [[nodiscard]] float GetFovYDegrees() const noexcept { return fovYDegrees; }
    SPARK_SCRIPT_BIND(get_near_plane)
    [[nodiscard]] float GetNearPlane() const noexcept { return nearPlane; }
    SPARK_SCRIPT_BIND(get_far_plane)
    [[nodiscard]] float GetFarPlane() const noexcept { return farPlane; }
    /** Half of the visible vertical span in world units (orthographic mode). */
    SPARK_SCRIPT_BIND(get_ortho_half_height)
    [[nodiscard]] float GetOrthoHalfHeight() const noexcept { return orthoHalfHeight; }
    SPARK_SCRIPT_BIND(get_priority)
    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    SPARK_SCRIPT_BIND(set_projection_mode)
    void SetProjectionMode(CameraProjectionMode mode) noexcept { projectionMode = mode; }
    SPARK_SCRIPT_BIND(set_fov_y_degrees)
    void SetFovYDegrees(float degrees) noexcept { fovYDegrees = degrees; }
    SPARK_SCRIPT_BIND(set_near_plane)
    void SetNearPlane(float z) noexcept { nearPlane = z; }
    SPARK_SCRIPT_BIND(set_far_plane)
    void SetFarPlane(float z) noexcept { farPlane = z; }
    SPARK_SCRIPT_BIND(set_ortho_half_height)
    void SetOrthoHalfHeight(float h) noexcept { orthoHalfHeight = h; }
    SPARK_SCRIPT_BIND(set_priority)
    void SetPriority(std::int32_t p) noexcept { priority = p; }
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(bool e) noexcept { enabled = e; }

    /** View matrix from the owner's world transform (inverse pose). */
    SPARK_SCRIPT_BIND(view_matrix)
    [[nodiscard]] Matrix4 ViewMatrix(const GameObject& owner) const noexcept;

    /** Projection for the given framebuffer aspect (width / height). */
    SPARK_SCRIPT_BIND(projection_matrix)
    [[nodiscard]] Matrix4 ProjectionMatrix(float aspect) const noexcept;

    SPARK_SCRIPT_BIND(view_projection)
    [[nodiscard]] Matrix4 ViewProjection(const GameObject& owner, float aspect) const noexcept;

    SPARK_SCRIPT_BIND(world_position)
    [[nodiscard]] Vector3 WorldPosition(const GameObject& owner) const noexcept;

    /** Normalized world right/up for billboards and particles. */
    SPARK_SCRIPT_BIND(billboard_basis_world)
    void BillboardBasisWorld(const GameObject& owner, Vector3& outRight, Vector3& outUp) const noexcept;

private:
    CameraProjectionMode projectionMode = CameraProjectionMode::Perspective;
    float fovYDegrees = 60.0F;
    float nearPlane = 0.1F;
    float farPlane = 500.0F;
    float orthoHalfHeight = 5.0F;
    std::int32_t priority = 0;
    bool enabled = true;
};

}  // namespace Spark
