#include "spark/ecs/components/physics/3d/PhysicsAttachedBody3DComponent.hpp"

namespace Spark {

void PhysicsAttachedBody3DComponent::Bind(GameObject& carrierIn, const Vector3& offsetIn) noexcept {
    carrier = &carrierIn;
    localOffset = offsetIn;
}

void PhysicsAttachedBody3DComponent::ClearBinding() noexcept {
    carrier = nullptr;
    localOffset = Vector3::Zero;
}

}  // namespace Spark
