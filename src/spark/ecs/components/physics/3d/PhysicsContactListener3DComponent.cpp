#include "spark/ecs/components/physics/3d/PhysicsContactListener3DComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"

namespace Spark {

void PhysicsContactListener3DComponent::OnSignal(
        GameObject& owner,
        const SignalId id,
        const SignalPayload& payload) {
    if (id != SignalId::Physics3DContact || payload.ptr == nullptr) {
        return;
    }
    const PhysicsContactReport3D* report = static_cast<const PhysicsContactReport3D*>(payload.ptr);
    if (report->dynamicObject != &owner) {
        return;
    }
    DeliverContact(*report);
}

}  // namespace Spark
