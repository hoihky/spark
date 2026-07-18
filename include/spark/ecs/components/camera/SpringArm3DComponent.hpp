#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class IEngineContext;

/**
 * Places the owner (typically a camera) along a yaw/pitch orbit behind a pivot.
 * Pair with <c>CameraFollow3DComponent</c> on the same object or a parent rig.
 */
class SpringArm3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpringArm3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 295; }

    [[nodiscard]] GameObject* GetPivotTarget() const noexcept { return pivotTarget; }
    [[nodiscard]] const Vector3& GetSocketOffset() const noexcept { return socketOffset; }
    [[nodiscard]] float GetArmLength() const noexcept { return armLength; }
    [[nodiscard]] float GetYawRadians() const noexcept { return yawRadians; }
    [[nodiscard]] float GetPitchRadians() const noexcept { return pitchRadians; }
    [[nodiscard]] float GetProbeRadius() const noexcept { return probeRadius; }
    [[nodiscard]] float GetMinArmLength() const noexcept { return minArmLength; }

    void SetPivotTarget(GameObject* o) noexcept { pivotTarget = o; }
    void SetSocketOffset(const Vector3& o) noexcept { socketOffset = o; }
    void SetArmLength(float l) noexcept { armLength = l; }
    void SetYawRadians(float y) noexcept { yawRadians = y; }
    void SetPitchRadians(float p) noexcept { pitchRadians = p; }
    void SetProbeRadius(float r) noexcept { probeRadius = r; }
    void SetMinArmLength(float l) noexcept { minArmLength = l; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    static void Tick(SpringArm3DComponent& arm, GameObject& owner) noexcept;

private:
    GameObject* pivotTarget = nullptr;
    Vector3 socketOffset{0.0F, 1.5F, 0.0F};
    float armLength = 4.0F;
    float yawRadians = 0.0F;
    float pitchRadians = -0.25F;
    float probeRadius = 0.2F;
    float minArmLength = 0.75F;
};

}  // namespace Spark
