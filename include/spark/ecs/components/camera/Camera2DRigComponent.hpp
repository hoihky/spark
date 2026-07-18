#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class IEngineContext;

/** Production 2D camera behaviors; drives the owner's <c>TransformComponent</c> each tick. */
enum class Camera2DRigMode : std::uint8_t {
    /** Pose is set externally; rig does not move the camera. */
    Manual = 0,
    /** Exponential smooth follow toward <c>target</c> + <c>targetOffset</c> (optional velocity look-ahead). */
    FollowTarget = 1,
    /** Follow then clamp the view center so the visible rect stays inside <c>bounds</c>. */
    BoundedFollow = 2,
};

/**
 * Follow / bounds / zoom rig for a sibling <c>Camera2DComponent</c>.
 * Runs after gameplay and physics so it can track dynamic targets cleanly.
 */
class Camera2DRigComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Camera2DRig;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 300; }

    Camera2DRigComponent() = default;

    [[nodiscard]] Camera2DRigMode GetMode() const noexcept { return mode; }
    [[nodiscard]] GameObject* GetTarget() const noexcept { return target; }
    [[nodiscard]] const Vector3& GetTargetOffset() const noexcept { return targetOffset; }
    [[nodiscard]] float GetFollowSmoothRate() const noexcept { return followSmoothRate; }
    [[nodiscard]] float GetLookAheadScale() const noexcept { return lookAheadScale; }
    [[nodiscard]] bool GetUseBounds() const noexcept { return useBounds; }
    [[nodiscard]] const Vector2& GetBoundsMin() const noexcept { return boundsMin; }
    [[nodiscard]] const Vector2& GetBoundsMax() const noexcept { return boundsMax; }
    [[nodiscard]] bool GetUseZoomLimits() const noexcept { return useZoomLimits; }
    [[nodiscard]] float GetZoomMinHalfExtentY() const noexcept { return zoomMinHalfExtentY; }
    [[nodiscard]] float GetZoomMaxHalfExtentY() const noexcept { return zoomMaxHalfExtentY; }

    void SetMode(Camera2DRigMode m) noexcept { mode = m; }
    void SetTarget(GameObject* o) noexcept { target = o; }
    void SetTargetOffset(const Vector3& o) noexcept { targetOffset = o; }
    void SetFollowSmoothRate(float r) noexcept { followSmoothRate = r; }
    void SetLookAheadScale(float s) noexcept { lookAheadScale = s; }
    void SetUseBounds(bool b) noexcept { useBounds = b; }
    void SetBoundsMin(const Vector2& v) noexcept { boundsMin = v; }
    void SetBoundsMax(const Vector2& v) noexcept { boundsMax = v; }
    void SetUseZoomLimits(bool b) noexcept { useZoomLimits = b; }
    void SetZoomMinHalfExtentY(float h) noexcept { zoomMinHalfExtentY = h; }
    void SetZoomMaxHalfExtentY(float h) noexcept { zoomMaxHalfExtentY = h; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    /** Manual tick when the world does not call <c>UpdateGameObjects</c> (shell demos). */
    static void Tick(
            Camera2DRigComponent& rig,
            GameObject& owner,
            float deltaSeconds,
            float framebufferAspect) noexcept;

private:
    Camera2DRigMode mode = Camera2DRigMode::Manual;
    GameObject* target = nullptr;
    Vector3 targetOffset{0.0F, 0.0F, 0.0F};
    /** Lerp factor per second: <c>min(1, rate * dt)</c>. Platformer default ~7.5. */
    float followSmoothRate = 7.5F;
    /** Multiplies target <c>Rigidbody2D</c> velocity for horizontal look-ahead (world units per m/s). */
    float lookAheadScale = 0.0F;
    bool useBounds = false;
    Vector2 boundsMin{-1.0e9F, -1.0e9F};
    Vector2 boundsMax{1.0e9F, 1.0e9F};
    bool useZoomLimits = false;
    float zoomMinHalfExtentY = 2.0F;
    float zoomMaxHalfExtentY = 40.0F;
};

}  // namespace Spark
