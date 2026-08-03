#pragma once

#include "spark/physics/core/ColliderMaterial.hpp"

namespace Spark {

struct StaticCollider2D;
class GameObject;

void ApplyPhysicsMaterial2DToStaticRecord(const GameObject& object, StaticCollider2D& rec) noexcept;
void ApplyPhysicsMaterial2DToCollider(const GameObject& object, ColliderMaterial& material) noexcept;

[[nodiscard]] float CombineRestitution2D(float a, float b) noexcept;
[[nodiscard]] float CombineFriction2D(float a, float b) noexcept;

}  // namespace Spark
