#pragma once

#include "spark/ecs/GameComponent.hpp"

namespace Spark {

class GameObject;

/** Spring-damper distance constraint between two sphere collider centers. */
class SpringJoint3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpringJoint3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SpringJoint3DComponent(GameObject* connectedBodyIn = nullptr, float restLengthIn = 1.0F) noexcept
            : connectedBody(connectedBodyIn), restLength(restLengthIn) {}

    [[nodiscard]] GameObject* GetConnectedBody() const noexcept { return connectedBody; }
    void SetConnectedBody(GameObject* o) noexcept { connectedBody = o; }

    [[nodiscard]] float GetRestLength() const noexcept { return restLength; }
    void SetRestLength(const float r) noexcept { restLength = r; }

    [[nodiscard]] float GetSpringStiffness() const noexcept { return springStiffness; }
    void SetSpringStiffness(const float k) noexcept { springStiffness = k; }

    [[nodiscard]] float GetDamping() const noexcept { return damping; }
    void SetDamping(const float d) noexcept { damping = d; }

private:
    GameObject* connectedBody = nullptr;
    float restLength = 1.0F;
    float springStiffness = 42.0F;
    float damping = 4.5F;
};

}  // namespace Spark
