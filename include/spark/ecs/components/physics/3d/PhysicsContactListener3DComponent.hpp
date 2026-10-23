#pragma once

#include "spark/core/Function.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/physics/simulation/PhysicsContactBroadcaster3D.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Optional gameplay hook for 3D contact reports. Also receives <c>SignalId::Physics3DContact</c> via <c>OnSignal</c>.
 */
class PhysicsContactListener3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PhysicsContactListener3D;

    using ContactHandler = Function<void(const PhysicsContactReport3D&)>;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void SetContactHandler(ContactHandler handlerIn) { contactHandler = MoveTemp(handlerIn); }

    void DeliverContact(const PhysicsContactReport3D& report) {
        if (contactHandler) {
            contactHandler(report);
        }
    }

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;

private:
    ContactHandler contactHandler{};
};

}  // namespace Spark
