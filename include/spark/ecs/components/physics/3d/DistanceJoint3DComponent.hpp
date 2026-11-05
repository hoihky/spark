#pragma once

#include "spark/ecs/GameComponent.hpp"

namespace Spark {

class GameObject;

/**
 * Keeps world-space distance between this object’s sphere center and <c>connectedBody</c>’s sphere center near
 * <c>restLength</c>. Both bodies should carry <c>SphereCollider3DComponent</c> + dynamic <c>Rigidbody3DComponent</c>
 * (or one end static / kinematic with only transform + collider for an anchor).
 */
class DistanceJoint3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::DistanceJoint3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit DistanceJoint3DComponent(GameObject* connectedBodyIn = nullptr, const float restLengthIn = 1.0F) noexcept
            : connectedBody(connectedBodyIn), restLength(restLengthIn) {}

    [[nodiscard]] GameObject* GetConnectedBody() const noexcept { return connectedBody; }
    void SetConnectedBody(GameObject* o) noexcept { connectedBody = o; }

    [[nodiscard]] float GetRestLength() const noexcept { return restLength; }
    void SetRestLength(const float r) noexcept { restLength = r; }

    /** Position correction scale per solver pass (0–1). */
    [[nodiscard]] float GetStiffness() const noexcept { return stiffness; }
    void SetStiffness(const float s) noexcept { stiffness = s; }

private:
    GameObject* connectedBody = nullptr;
    float restLength = 1.0F;
    float stiffness = 0.55F;
};

}  // namespace Spark
