#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;

/** Pin constraint: keeps anchor points coincident in world XY. */
class HingeJoint2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::HingeJoint2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit HingeJoint2DComponent(GameObject* connectedBodyIn = nullptr) noexcept : connectedBody(connectedBodyIn) {}

    SPARK_SCRIPT_BIND(get_connected_body)
    [[nodiscard]] GameObject* GetConnectedBody() const noexcept { return connectedBody; }
    SPARK_SCRIPT_BIND(set_connected_body)
    void SetConnectedBody(GameObject* o) noexcept { connectedBody = o; }

    SPARK_SCRIPT_BIND(get_local_anchor_a)
    [[nodiscard]] const Vector2& GetLocalAnchorA() const noexcept { return localAnchorA; }
    SPARK_SCRIPT_BIND(set_local_anchor_a)
    void SetLocalAnchorA(const Vector2& a) noexcept { localAnchorA = a; }

    SPARK_SCRIPT_BIND(get_local_anchor_b)
    [[nodiscard]] const Vector2& GetLocalAnchorB() const noexcept { return localAnchorB; }
    SPARK_SCRIPT_BIND(set_local_anchor_b)
    void SetLocalAnchorB(const Vector2& b) noexcept { localAnchorB = b; }

    SPARK_SCRIPT_BIND(get_stiffness)
    [[nodiscard]] float GetStiffness() const noexcept { return stiffness; }
    SPARK_SCRIPT_BIND(set_stiffness)
    void SetStiffness(const float s) noexcept { stiffness = s; }

private:
    GameObject* connectedBody = nullptr;
    Vector2 localAnchorA{Vector2::Zero};
    Vector2 localAnchorB{Vector2::Zero};
    float stiffness = 0.65F;
};

}  // namespace Spark
