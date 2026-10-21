#include "spark/physics/simulation/PhysicsContactBroadcaster3D.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/colliders/DynamicBody3D.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void PhysicsContactBroadcaster3D::BeginSubstep() noexcept {
    pending.Clear();
}

void PhysicsContactBroadcaster3D::QueueContact(const PendingContact& contact) noexcept {
    if (!enabled || contact.dynamicId == 0) {
        return;
    }
    pending.PushBack(contact);
}

void PhysicsContactBroadcaster3D::NotifyStaticContact(
        DynamicBody3D& body,
        const Collider3D& staticCollider,
        const std::uint32_t staticIndex,
        const float normalX,
        const float normalY,
        const float normalZ,
        const float penetration) noexcept {
    if (!enabled || body.obj == nullptr) {
        return;
    }
    PendingContact pendingContact{};
    pendingContact.dynamicId = body.obj->GetId();
    pendingContact.otherIsStatic = true;
    pendingContact.staticIndex = staticIndex;
    pendingContact.penetration = penetration;
    pendingContact.normal = {normalX, normalY, normalZ};
    pendingContact.point = body.collider.GetTrackingPoint();
    if (GameObject* staticOwner = staticCollider.GetOwner()) {
        pendingContact.otherId = staticOwner->GetId();
    }
    QueueContact(pendingContact);
}

void PhysicsContactBroadcaster3D::NotifyDynamicContact(
        DynamicBody3D& bodyA,
        DynamicBody3D& bodyB,
        const float normalX,
        const float normalY,
        const float normalZ,
        const float penetration) noexcept {
    if (!enabled || bodyA.obj == nullptr || bodyB.obj == nullptr) {
        return;
    }
    PendingContact toA{};
    toA.dynamicId = bodyA.obj->GetId();
    toA.otherId = bodyB.obj->GetId();
    toA.otherIsStatic = false;
    toA.penetration = penetration;
    toA.normal = {normalX, normalY, normalZ};
    toA.point = bodyA.collider.GetTrackingPoint();
    QueueContact(toA);

    PendingContact toB{};
    toB.dynamicId = bodyB.obj->GetId();
    toB.otherId = bodyA.obj->GetId();
    toB.otherIsStatic = false;
    toB.penetration = penetration;
    toB.normal = {-normalX, -normalY, -normalZ};
    toB.point = bodyB.collider.GetTrackingPoint();
    QueueContact(toB);
}

void PhysicsContactBroadcaster3D::Flush(GameWorld& world) noexcept {
    if (!enabled) {
        pending.Clear();
        return;
    }

    for (std::size_t i = 0; i < pending.GetSize(); ++i) {
        const PendingContact& c = pending[i];
        GameObject* dynamicObject = world.FindGameObjectById(c.dynamicId);
        if (dynamicObject == nullptr) {
            continue;
        }
        GameObject* otherObject = c.otherId != 0 ? world.FindGameObjectById(c.otherId) : nullptr;

        PhysicsContactReport3D report{};
        report.dynamicObject = dynamicObject;
        report.otherObject = otherObject;
        report.normal = c.normal;
        report.pointOnDynamic = c.point;
        report.penetration = c.penetration;
        report.staticColliderIndex = c.staticIndex;
        report.otherIsStatic = c.otherIsStatic;

        SignalPayload payload{};
        payload.ptr = &report;
        payload.a = c.otherId;
        payload.b = c.staticIndex;
        dynamicObject->EmitSignal(SignalId::Physics3DContact, payload, nullptr);
    }
    pending.Clear();
}

}  // namespace Spark
