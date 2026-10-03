#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;

/** Pin constraint between two dynamic bodies at local anchor offsets (revolute-style position lock). */
class HingeJoint3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::HingeJoint3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit HingeJoint3DComponent(GameObject* connectedBodyIn = nullptr) noexcept : connectedBody(connectedBodyIn) {}

    SPARK_SCRIPT_BIND(get_connected_body)
    [[nodiscard]] GameObject* GetConnectedBody() const noexcept { return connectedBody; }
    SPARK_SCRIPT_BIND(set_connected_body)
    void SetConnectedBody(GameObject* o) noexcept { connectedBody = o; }

    SPARK_SCRIPT_BIND(get_local_anchor_a)
    [[nodiscard]] const Vector3& GetLocalAnchorA() const noexcept { return localAnchorA; }
    SPARK_SCRIPT_BIND(set_local_anchor_a)
    void SetLocalAnchorA(const Vector3& a) noexcept { localAnchorA = a; }

    SPARK_SCRIPT_BIND(get_local_anchor_b)
    [[nodiscard]] const Vector3& GetLocalAnchorB() const noexcept { return localAnchorB; }
    SPARK_SCRIPT_BIND(set_local_anchor_b)
    void SetLocalAnchorB(const Vector3& b) noexcept { localAnchorB = b; }

    SPARK_SCRIPT_BIND(get_stiffness)
    [[nodiscard]] float GetStiffness() const noexcept { return stiffness; }
    SPARK_SCRIPT_BIND(set_stiffness)
    void SetStiffness(const float s) noexcept { stiffness = s; }

private:
    GameObject* connectedBody = nullptr;
    Vector3 localAnchorA{Vector3::Zero};
    Vector3 localAnchorB{Vector3::Zero};
    float stiffness = 0.65F;
};

}  // namespace Spark
