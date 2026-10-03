#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;

/** Spring-damper distance constraint between two sphere collider centers. */
class SpringJoint3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpringJoint3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SpringJoint3DComponent(GameObject* connectedBodyIn = nullptr, float restLengthIn = 1.0F) noexcept
            : connectedBody(connectedBodyIn), restLength(restLengthIn) {}

    SPARK_SCRIPT_BIND(get_connected_body)
    [[nodiscard]] GameObject* GetConnectedBody() const noexcept { return connectedBody; }
    SPARK_SCRIPT_BIND(set_connected_body)
    void SetConnectedBody(GameObject* o) noexcept { connectedBody = o; }

    SPARK_SCRIPT_BIND(get_rest_length)
    [[nodiscard]] float GetRestLength() const noexcept { return restLength; }
    SPARK_SCRIPT_BIND(set_rest_length)
    void SetRestLength(const float r) noexcept { restLength = r; }

    SPARK_SCRIPT_BIND(get_spring_stiffness)
    [[nodiscard]] float GetSpringStiffness() const noexcept { return springStiffness; }
    SPARK_SCRIPT_BIND(set_spring_stiffness)
    void SetSpringStiffness(const float k) noexcept { springStiffness = k; }

    SPARK_SCRIPT_BIND(get_damping)
    [[nodiscard]] float GetDamping() const noexcept { return damping; }
    SPARK_SCRIPT_BIND(set_damping)
    void SetDamping(const float d) noexcept { damping = d; }

private:
    GameObject* connectedBody = nullptr;
    float restLength = 1.0F;
    float springStiffness = 42.0F;
    float damping = 4.5F;
};

}  // namespace Spark
