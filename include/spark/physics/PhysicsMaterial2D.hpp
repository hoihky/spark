#pragma once

#include "spark/physics/Collision2D.hpp"

namespace Spark {

class GameObject;

void ApplyPhysicsMaterial2DToStaticRecord(const GameObject& object, StaticCollider2D& rec) noexcept;

[[nodiscard]] float CombineRestitution2D(float a, float b) noexcept;
[[nodiscard]] float CombineFriction2D(float a, float b) noexcept;

}  // namespace Spark
