#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

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

    [[nodiscard]] CameraProjectionMode GetProjectionMode() const noexcept { return projectionMode; }
    [[nodiscard]] float GetFovYDegrees() const noexcept { return fovYDegrees; }
    [[nodiscard]] float GetNearPlane() const noexcept { return nearPlane; }
    [[nodiscard]] float GetFarPlane() const noexcept { return farPlane; }
    /** Half of the visible vertical span in world units (orthographic mode). */
    [[nodiscard]] float GetOrthoHalfHeight() const noexcept { return orthoHalfHeight; }
    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetProjectionMode(CameraProjectionMode mode) noexcept { projectionMode = mode; }
    void SetFovYDegrees(float degrees) noexcept { fovYDegrees = degrees; }
    void SetNearPlane(float z) noexcept { nearPlane = z; }
    void SetFarPlane(float z) noexcept { farPlane = z; }
    void SetOrthoHalfHeight(float h) noexcept { orthoHalfHeight = h; }
    void SetPriority(std::int32_t p) noexcept { priority = p; }
    void SetEnabled(bool e) noexcept { enabled = e; }

    /** View matrix from the owner's world transform (inverse pose). */
    [[nodiscard]] Matrix4 ViewMatrix(const GameObject& owner) const noexcept;

    /** Projection for the given framebuffer aspect (width / height). */
    [[nodiscard]] Matrix4 ProjectionMatrix(float aspect) const noexcept;

    [[nodiscard]] Matrix4 ViewProjection(const GameObject& owner, float aspect) const noexcept;

    [[nodiscard]] Vector3 WorldPosition(const GameObject& owner) const noexcept;

    /** Normalized world right/up for billboards and particles. */
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
