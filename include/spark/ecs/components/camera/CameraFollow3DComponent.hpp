#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class IEngineContext;

/** Third-person style follow rig for a sibling <c>CameraComponent</c>. */
class CameraFollow3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::CameraFollow3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 300; }

    SPARK_SCRIPT_BIND(get_target)
    [[nodiscard]] GameObject* GetTarget() const noexcept { return target; }
    SPARK_SCRIPT_BIND(get_target_offset)
    [[nodiscard]] const Vector3& GetTargetOffset() const noexcept { return targetOffset; }
    SPARK_SCRIPT_BIND(get_follow_smooth_rate)
    [[nodiscard]] float GetFollowSmoothRate() const noexcept { return followSmoothRate; }
    SPARK_SCRIPT_BIND(get_look_at_target)
    [[nodiscard]] bool GetLookAtTarget() const noexcept { return lookAtTarget; }

    SPARK_SCRIPT_BIND(set_target)
    void SetTarget(GameObject* o) noexcept { target = o; }
    SPARK_SCRIPT_BIND(set_target_offset)
    void SetTargetOffset(const Vector3& o) noexcept { targetOffset = o; }
    SPARK_SCRIPT_BIND(set_follow_smooth_rate)
    void SetFollowSmoothRate(float r) noexcept { followSmoothRate = r; }
    SPARK_SCRIPT_BIND(set_look_at_target)
    void SetLookAtTarget(bool v) noexcept { lookAtTarget = v; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(tick)
    static void Tick(CameraFollow3DComponent& rig, GameObject& owner, float deltaSeconds) noexcept;

private:
    GameObject* target = nullptr;
    Vector3 targetOffset{0.0F, 1.6F, 0.0F};
    float followSmoothRate = 8.0F;
    bool lookAtTarget = true;
};

}  // namespace Spark
