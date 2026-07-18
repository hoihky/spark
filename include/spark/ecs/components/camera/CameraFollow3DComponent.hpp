#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class IEngineContext;

/** Third-person style follow rig for a sibling <c>CameraComponent</c>. */
class CameraFollow3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::CameraFollow3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 300; }

    [[nodiscard]] GameObject* GetTarget() const noexcept { return target; }
    [[nodiscard]] const Vector3& GetTargetOffset() const noexcept { return targetOffset; }
    [[nodiscard]] float GetFollowSmoothRate() const noexcept { return followSmoothRate; }
    [[nodiscard]] bool GetLookAtTarget() const noexcept { return lookAtTarget; }

    void SetTarget(GameObject* o) noexcept { target = o; }
    void SetTargetOffset(const Vector3& o) noexcept { targetOffset = o; }
    void SetFollowSmoothRate(float r) noexcept { followSmoothRate = r; }
    void SetLookAtTarget(bool v) noexcept { lookAtTarget = v; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    static void Tick(CameraFollow3DComponent& rig, GameObject& owner, float deltaSeconds) noexcept;

private:
    GameObject* target = nullptr;
    Vector3 targetOffset{0.0F, 1.6F, 0.0F};
    float followSmoothRate = 8.0F;
    bool lookAtTarget = true;
};

}  // namespace Spark
