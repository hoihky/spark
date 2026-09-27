#pragma once

namespace Spark {

class GameObject;

/**
 * Shared damage application for 2D combat systems (Strategy: one path for hitboxes and projectiles).
 * Respects <c>Hurtbox2DComponent</c> invulnerability and routes through <c>DamageableComponent</c> /
 * <c>HealthComponent</c>.
 *
 * @return Applied damage amount (0 when blocked or no health).
 */
float TryApplyCombatDamage2D(GameObject& target, GameObject* instigator, float rawDamage) noexcept;

}  // namespace Spark
