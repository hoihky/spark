#include "spark/physics/core/ColliderMaterial.hpp"

#include "spark/physics/Collision2D.hpp"
#include "spark/physics/Collision3D.hpp"

namespace Spark {

ColliderMaterial ColliderMaterial::FromStaticCollider2D(const StaticCollider2D& collider) noexcept {
    ColliderMaterial material{};
    material.isDefined = collider.hasMaterial;
    material.restitution = collider.restitution;
    material.dynamicFriction = collider.dynamicFriction;
    material.staticFriction = collider.dynamicFriction;
    return material;
}

ColliderMaterial ColliderMaterial::FromStaticCollider3D(const StaticCollider3DSim& collider) noexcept {
    ColliderMaterial material{};
    material.isDefined = collider.hasMaterial;
    material.restitution = collider.restitution;
    material.staticFriction = collider.staticFriction;
    material.dynamicFriction = collider.dynamicFriction;
    return material;
}

}  // namespace Spark
