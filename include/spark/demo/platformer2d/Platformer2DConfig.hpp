#pragma once

#include "spark/physics/CollisionFilter2D.hpp"

namespace Spark::Platformer2D {

/** Tunable gameplay constants for the teaching demo (single place to balance the level). */
struct Config {
    static constexpr float kPlayerMaxHealth = 100.0F;
    static constexpr float kEnemyBulletDamage = 5.0F;
    static constexpr float kFallDamage = 5.0F;
    static constexpr float kPlayerHurtCooldownSeconds = 0.55F;

    static constexpr float kPlayerHalfW = 0.40F;
    static constexpr float kPlayerHalfH = 0.54F;

    static constexpr float kEnemyHalfW = 0.36F;
    static constexpr float kEnemyHalfH = 0.42F;
    static constexpr float kEnemyDrawScale = 0.88F;
    static constexpr float kEnemyPatrolSpeed = 2.2F;
    static constexpr float kEnemyBobAmplitude = 0.05F;
    static constexpr float kEnemyShootRangeX = 14.0F;
    static constexpr float kEnemyShootRangeY = 7.5F;

    static constexpr int kEnemyCount = 4;
    static constexpr int kMaxEnemyBullets = 12;
    static constexpr int kMaxPlayerBullets = 10;

    static constexpr float kPlayerBulletSpeed = 13.5F;
    static constexpr float kEnemyBulletSpeed = 7.0F;
    static constexpr float kBulletLifetimeSeconds = 3.6F;
    static constexpr float kPlayerBulletDrawScale = 0.40F;
    static constexpr float kEnemyBulletDrawScale = 0.42F;
    static constexpr float kPlayerBulletHalfW = 0.050F;
    static constexpr float kPlayerBulletHalfH = 0.032F;
    static constexpr float kEnemyBulletHalfW = 0.055F;
    static constexpr float kEnemyBulletHalfH = 0.035F;

    static constexpr int kExplosionMaxParticles = 72;
    static constexpr int kExplosionBurstCount = 10;

    static constexpr std::uint16_t kGemHurtboxCategoryBits = Spark::CollisionFilter2D::LayerBit(1);
    static constexpr std::uint16_t kEnemyHurtboxCategoryBits = Spark::CollisionFilter2D::LayerBit(3);

    /** World XY plus patrol span (x, y, patrolMinX, patrolMaxX). */
    static constexpr float kEnemySpawns[kEnemyCount][4] = {
            {-4.5F, 1.35F, -6.8F, -1.5F},
            {7.4F, 2.82F, 6.2F, 8.8F},
            {21.0F, 1.48F, 18.5F, 23.5F},
            {25.0F, 5.62F, 23.2F, 26.8F},
    };
};

}  // namespace Spark::Platformer2D
