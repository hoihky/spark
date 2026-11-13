#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include <cstdint>

namespace Spark {

inline constexpr std::uint32_t kPhysicsContactNoStaticColliderIndex = 0xFFFFFFFFu;

class Collider3D;
class GameObject;
class GameWorld;
struct DynamicBody3D;

/** Payload for <c>SignalId::Physics3DContact</c> (valid only during synchronous dispatch). */
struct PhysicsContactReport3D {
    GameObject* dynamicObject = nullptr;
    GameObject* otherObject = nullptr;
    Vector3 normal{Vector3::UnitY};
    Vector3 pointOnDynamic{Vector3::Zero};
    float penetration = 0.0F;
    std::uint32_t staticColliderIndex = kPhysicsContactNoStaticColliderIndex;
    bool otherIsStatic = true;
};

/**
 * Collects contact events during <c>ContactResolver3D</c> and dispatches signals to dynamic bodies.
 * Owned by <c>PhysicsWorld3D</c>.
 */
class PhysicsContactBroadcaster3D {
public:
    void BeginSubstep() noexcept;
    void NotifyStaticContact(
            DynamicBody3D& body,
            const Collider3D& staticCollider,
            std::uint32_t staticIndex,
            float normalX,
            float normalY,
            float normalZ,
            float penetration) noexcept;
    void NotifyDynamicContact(
            DynamicBody3D& bodyA,
            DynamicBody3D& bodyB,
            float normalX,
            float normalY,
            float normalZ,
            float penetration) noexcept;
    void Flush(GameWorld& world) noexcept;

    void SetEnabled(bool enabledIn) noexcept { enabled = enabledIn; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

private:
    struct PendingContact {
        std::uint64_t dynamicId = 0;
        std::uint64_t otherId = 0;
        Vector3 normal{Vector3::UnitY};
        Vector3 point{Vector3::Zero};
        float penetration = 0.0F;
        std::uint32_t staticIndex = kPhysicsContactNoStaticColliderIndex;
        bool otherIsStatic = true;
    };

    void QueueContact(const PendingContact& contact) noexcept;

    bool enabled = true;
    Array<PendingContact> pending{};
};

}  // namespace Spark
