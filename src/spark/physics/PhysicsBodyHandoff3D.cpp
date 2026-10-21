#include "spark/physics/PhysicsBodyHandoff3D.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsAttachedBody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 TransformPoint(const Matrix4& worldMatrix, const Vector3& local) noexcept {
    const Vector4 p = worldMatrix * Vector4(local.x, local.y, local.z, 1.0F);
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

}  // namespace

void PhysicsBodyHandoff3D::AttachRigidbodyToCarrier(
        GameWorld& world,
        GameObject& carried,
        GameObject& carrier,
        const Vector3& localOffset) noexcept {
    (void)world;
    auto* rb = carried.GetComponent<Rigidbody3DComponent>();
    auto* tr = carried.GetComponent<TransformComponent>();
    if (rb == nullptr || tr == nullptr || &carried == &carrier) {
        return;
    }
    auto* attachment = carried.GetComponent<PhysicsAttachedBody3DComponent>();
    if (attachment == nullptr) {
        attachment = carried.AddComponent<PhysicsAttachedBody3DComponent>();
    }
    attachment->Bind(carrier, localOffset);
    rb->SetBodyType(RigidbodyBodyType3D::Kinematic);
    rb->SetVelocity(Vector3::Zero);
    rb->SetAngularVelocity(Vector3::Zero);
    SynchronizeAttachedBodies(world);
}

void PhysicsBodyHandoff3D::ReleaseRigidbody(GameObject& carried, const Vector3& worldImpulse) noexcept {
    auto* rb = carried.GetComponent<Rigidbody3DComponent>();
    auto* tr = carried.GetComponent<TransformComponent>();
    if (rb == nullptr) {
        return;
    }
    if (auto* attachment = carried.GetComponent<PhysicsAttachedBody3DComponent>()) {
        attachment->ClearBinding();
    }
    rb->SetBodyType(RigidbodyBodyType3D::Dynamic);
    if (tr != nullptr) {
        Vector3 pos = tr->GetLocalTransform().translation;
        Vector3 nudge = worldImpulse;
        if (nudge.LengthSquared() > 1.0e-6F) {
            nudge = nudge.Normalized() * 0.22F;
        } else {
            nudge = Vector3{0.0F, 0.18F, 0.0F};
        }
        tr->SetTranslation(pos + nudge);
    }
    rb->SetVelocity(worldImpulse);
    rb->SetAngularVelocity(Vector3::Zero);
}

void PhysicsBodyHandoff3D::SynchronizeAttachedBodies(GameWorld& world) noexcept {
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        auto* attachment = object->GetComponent<PhysicsAttachedBody3DComponent>();
        if (attachment == nullptr || !attachment->IsAttached()) {
            return;
        }
        GameObject* carrier = attachment->GetCarrier();
        auto* carrierTr = carrier != nullptr ? carrier->GetComponent<TransformComponent>() : nullptr;
        auto* tr = object->GetComponent<TransformComponent>();
        if (carrier == nullptr || carrierTr == nullptr || tr == nullptr) {
            return;
        }
        const Vector3 worldPos = TransformPoint(carrier->GetWorldMatrix(), attachment->GetLocalOffset());
        tr->SetTranslation(worldPos);
    });
}

}  // namespace Spark
