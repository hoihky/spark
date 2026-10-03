#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(get_pivot_target)
    [[nodiscard]] GameObject* GetPivotTarget() const noexcept { return pivotTarget; }
    SPARK_SCRIPT_BIND(get_socket_offset)
    [[nodiscard]] const Vector3& GetSocketOffset() const noexcept { return socketOffset; }
    SPARK_SCRIPT_BIND(get_arm_length)
    [[nodiscard]] float GetArmLength() const noexcept { return armLength; }
    SPARK_SCRIPT_BIND(get_yaw_radians)
    [[nodiscard]] float GetYawRadians() const noexcept { return yawRadians; }
    SPARK_SCRIPT_BIND(get_pitch_radians)
    [[nodiscard]] float GetPitchRadians() const noexcept { return pitchRadians; }
    SPARK_SCRIPT_BIND(get_probe_radius)
    [[nodiscard]] float GetProbeRadius() const noexcept { return probeRadius; }
    SPARK_SCRIPT_BIND(get_min_arm_length)
    [[nodiscard]] float GetMinArmLength() const noexcept { return minArmLength; }

    SPARK_SCRIPT_BIND(set_pivot_target)
    void SetPivotTarget(GameObject* o) noexcept { pivotTarget = o; }
    SPARK_SCRIPT_BIND(set_socket_offset)
    void SetSocketOffset(const Vector3& o) noexcept { socketOffset = o; }
    SPARK_SCRIPT_BIND(set_arm_length)
    void SetArmLength(float l) noexcept { armLength = l; }
    SPARK_SCRIPT_BIND(set_yaw_radians)
    void SetYawRadians(float y) noexcept { yawRadians = y; }
    SPARK_SCRIPT_BIND(set_pitch_radians)
    void SetPitchRadians(float p) noexcept { pitchRadians = p; }
    SPARK_SCRIPT_BIND(set_probe_radius)
    void SetProbeRadius(float r) noexcept { probeRadius = r; }
    SPARK_SCRIPT_BIND(set_min_arm_length)
    void SetMinArmLength(float l) noexcept { minArmLength = l; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(tick)
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
