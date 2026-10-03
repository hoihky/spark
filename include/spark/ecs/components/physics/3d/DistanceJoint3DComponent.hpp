#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

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

    SPARK_SCRIPT_BIND(get_connected_body)
    [[nodiscard]] GameObject* GetConnectedBody() const noexcept { return connectedBody; }
    SPARK_SCRIPT_BIND(set_connected_body)
    void SetConnectedBody(GameObject* o) noexcept { connectedBody = o; }

    SPARK_SCRIPT_BIND(get_rest_length)
    [[nodiscard]] float GetRestLength() const noexcept { return restLength; }
    SPARK_SCRIPT_BIND(set_rest_length)
    void SetRestLength(const float r) noexcept { restLength = r; }

    /** Position correction scale per solver pass (0–1). */
    SPARK_SCRIPT_BIND(get_stiffness)
    [[nodiscard]] float GetStiffness() const noexcept { return stiffness; }
    SPARK_SCRIPT_BIND(set_stiffness)
    void SetStiffness(const float s) noexcept { stiffness = s; }

private:
    GameObject* connectedBody = nullptr;
    float restLength = 1.0F;
    float stiffness = 0.55F;
};

}  // namespace Spark
