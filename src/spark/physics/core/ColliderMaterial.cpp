#include "spark/physics/core/ColliderMaterial.hpp"

#include "spark/physics/2d/Collision2D.hpp"
#include "spark/physics/3d/Collision3D.hpp"

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
