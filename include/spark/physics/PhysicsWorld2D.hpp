#pragma once

#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameWorld;

/** Vertical gravity acceleration (world +Y is up); typical value -32. */
struct PhysicsWorld2DSettings {
    float gravityY = -32.0F;
    float maxFallSpeed = 46.0F;
    /**
     * When true, overlapping **non-trigger** dynamic pairs receive a single lightweight positional separation
     * after static resolution (circle–circle, box–box, box–circle). Triggers never separate; they only emit
     * <c>Physics2DTriggerOverlap</c>. A second static pass is not run (fast-moving bodies may re-penetrate statics).
     */
    bool resolveDynamicVsDynamic = false;
};

/**
 * Integrates dynamic rigidbodies (Rigidbody2D Dynamic + Transform + BoxCollider2D and/or CircleCollider2D).
 * If a dynamic body has both colliders, the circle is used for simulation. Static geometry is any
 * BoxCollider2D and/or CircleCollider2D on objects without a dynamic rigidbody (or with Static/Kinematic rigidbody).
 *
 * Collision filtering: each collider exposes category/mask bitmasks; contacts resolve only when both directions agree
 * (`CollisionFilter2D::ShouldCollide`).
 *
 * Triggers: colliders with `GetIsTrigger() == true` never apply separation in dynamic-vs-static resolution; overlap is
 * reported after resolution via `SignalId::Physics2DTriggerOverlap` on each participating trigger GameObject (payload:
 * other object pointer in `ptr`, id in `a`, baked static index in `b`).
 *
 * Each step, static colliders are baked into `StaticCollider2D` (conservative AABB + shape kind + filter bits) and a
 * uniform-cell `SpatialHashGrid2D`; dynamics query the hash then run narrow-phase (box vs box, box vs circle, circle
 * vs box, circle vs circle).
 *
 * After all dynamics are integrated and resolved against statics, **dynamic–dynamic** overlaps are evaluated (same
 * layer rules) using the same uniform-cell spatial hash as statics (payload = dynamic body index). Overlapping
 * triggers emit `SignalId::Physics2DTriggerOverlap` with `payload.b` =
 * `kPhysics2DTriggerOverlapNoStaticIndex`. Optional `resolveDynamicVsDynamic` applies a single shallow separation for
 * solid–solid pairs. World queries against statics live in `spark/physics/PhysicsQueries2D.hpp`.
 */
void SimulatePhysics2D(GameWorld& world, const FrameTiming& timing, const PhysicsWorld2DSettings& settings = {});

}  // namespace Spark
