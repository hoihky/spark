#pragma once

#include <cstdint>

namespace Spark {

/**
 * Built-in signals broadcast on a GameObject to sibling GameComponents.
 * Values below UserBase are engine-reserved; game code may use UserBase + n.
 */
enum class SignalId : std::uint32_t {
    None = 0,
    TransformChanged = 1,
    MeshDirty = 2,
    CollisionBoundsDirty = 3,
    /**
     * 2D trigger overlap: <c>ptr</c> = other <c>GameObject*</c>, <c>a</c> = other object's id.
     * <c>b</c> = baked static collider index in <c>SimulatePhysics2D</c>'s static array, or
     * <c>kPhysics2DTriggerOverlapNoStaticIndex</c> when the other body is dynamic (see <c>spark/physics/PhysicsQueries2D.hpp</c>).
     */
    Physics2DTriggerOverlap = 4,
    UserBase = 0x10000,
};

/** Small untyped payload for cross-component signals (typed data via ptr or packed u64). */
struct SignalPayload {
    const void* ptr = nullptr;
    std::uint64_t a = 0;
    std::uint64_t b = 0;
};

}  // namespace Spark
