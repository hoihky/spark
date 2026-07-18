#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

namespace Spark {

class GameObject;

/** Pin constraint: keeps anchor points coincident in world XY. */
class HingeJoint2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::HingeJoint2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit HingeJoint2DComponent(GameObject* connectedBodyIn = nullptr) noexcept : connectedBody(connectedBodyIn) {}

    [[nodiscard]] GameObject* GetConnectedBody() const noexcept { return connectedBody; }
    void SetConnectedBody(GameObject* o) noexcept { connectedBody = o; }

    [[nodiscard]] const Vector2& GetLocalAnchorA() const noexcept { return localAnchorA; }
    void SetLocalAnchorA(const Vector2& a) noexcept { localAnchorA = a; }

    [[nodiscard]] const Vector2& GetLocalAnchorB() const noexcept { return localAnchorB; }
    void SetLocalAnchorB(const Vector2& b) noexcept { localAnchorB = b; }

    [[nodiscard]] float GetStiffness() const noexcept { return stiffness; }
    void SetStiffness(const float s) noexcept { stiffness = s; }

private:
    GameObject* connectedBody = nullptr;
    Vector2 localAnchorA{Vector2::Zero};
    Vector2 localAnchorB{Vector2::Zero};
    float stiffness = 0.65F;
};

}  // namespace Spark
