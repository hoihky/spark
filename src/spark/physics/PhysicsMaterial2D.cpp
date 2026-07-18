#include "spark/physics/PhysicsMaterial2D.hpp"

#include "spark/ecs/components/physics/2d/PhysicsMaterial2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"

#include <cmath>

namespace Spark {

void ApplyPhysicsMaterial2DToStaticRecord(const GameObject& object, StaticCollider2D& rec) noexcept {
    if (const PhysicsMaterial2DComponent* mat = object.GetComponent<PhysicsMaterial2DComponent>()) {
        rec.hasMaterial = true;
        rec.restitution = mat->GetRestitution();
        rec.dynamicFriction = mat->GetDynamicFriction();
    }
}

float CombineRestitution2D(const float a, const float b) noexcept {
    return std::sqrt(std::max(0.0F, a) * std::max(0.0F, b));
}

float CombineFriction2D(const float a, const float b) noexcept {
    return std::sqrt(std::max(0.0F, a) * std::max(0.0F, b));
}

}  // namespace Spark
