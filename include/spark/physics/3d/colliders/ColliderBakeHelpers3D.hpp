#pragma once

#include "spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/core/ColliderMaterial.hpp"

namespace Spark {

inline ColliderMaterial MaterialFromGameObject3D(const GameObject& object) noexcept {
    ColliderMaterial material{};
    if (const PhysicsMaterial3DComponent* mat = object.GetComponent<PhysicsMaterial3DComponent>()) {
        material.isDefined = true;
        material.restitution = mat->GetRestitution();
        material.staticFriction = mat->GetStaticFriction();
        material.dynamicFriction = mat->GetDynamicFriction();
    }
    return material;
}

}  // namespace Spark
