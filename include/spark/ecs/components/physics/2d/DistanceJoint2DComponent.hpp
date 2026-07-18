#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

namespace Spark {

class GameObject;

/** Keeps world distance between two 2D bodies near <c>restLength</c>. */
class DistanceJoint2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::DistanceJoint2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit DistanceJoint2DComponent(GameObject* connectedBodyIn = nullptr, float restLengthIn = 1.0F) noexcept
            : connectedBody(connectedBodyIn), restLength(restLengthIn) {}

    [[nodiscard]] GameObject* GetConnectedBody() const noexcept { return connectedBody; }
    void SetConnectedBody(GameObject* o) noexcept { connectedBody = o; }

    [[nodiscard]] float GetRestLength() const noexcept { return restLength; }
    void SetRestLength(const float r) noexcept { restLength = r; }

    [[nodiscard]] const Vector2& GetLocalAnchorA() const noexcept { return localAnchorA; }
    void SetLocalAnchorA(const Vector2& a) noexcept { localAnchorA = a; }

    [[nodiscard]] const Vector2& GetLocalAnchorB() const noexcept { return localAnchorB; }
    void SetLocalAnchorB(const Vector2& b) noexcept { localAnchorB = b; }

    [[nodiscard]] float GetStiffness() const noexcept { return stiffness; }
    void SetStiffness(const float s) noexcept { stiffness = s; }

private:
    GameObject* connectedBody = nullptr;
    float restLength = 1.0F;
    Vector2 localAnchorA{Vector2::Zero};
    Vector2 localAnchorB{Vector2::Zero};
    float stiffness = 0.55F;
};

}  // namespace Spark
