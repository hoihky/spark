#include "spark/physics/PhysicsJoints.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/DistanceJoint2DComponent.hpp"
#include "spark/ecs/components/physics/2d/HingeJoint2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/3d/HingeJoint3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SpringJoint3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector2 WorldAnchor2D(const GameObject& owner, const Vector2& localAnchor) noexcept {
    const Vector3 w = owner.GetWorldMatrix().TransformPoint({localAnchor.x, localAnchor.y, 0.0F});
    return {w.x, w.y};
}

void ApplyPositionDelta2D(
        TransformComponent& tr,
        Rigidbody2DComponent* rb,
        const float dx,
        const float dy,
        const float weight) noexcept {
    if (weight <= 1.0e-8F) {
        return;
    }
    Vector3 pos = tr.GetLocalTransform().translation;
    pos.x += dx * weight;
    pos.y += dy * weight;
    tr.SetTranslation(pos);
    if (rb != nullptr && rb->GetBodyType() == RigidbodyBodyType2D::Dynamic) {
        Vector2 v = rb->GetVelocity();
        v.x += dx * weight * 0.25F;
        v.y += dy * weight * 0.25F;
        rb->SetVelocity(v);
    }
}

[[nodiscard]] Vector3 WorldAnchor3D(const GameObject& owner, const Vector3& localAnchor) noexcept {
    return owner.GetWorldMatrix().TransformPoint(localAnchor);
}

void ApplyPositionDelta3D(
        TransformComponent& tr,
        Rigidbody3DComponent* rb,
        const float nx,
        const float ny,
        const float nz,
        const float correction,
        const float weight) noexcept {
    if (weight <= 1.0e-8F) {
        return;
    }
    Vector3 pos = tr.GetLocalTransform().translation;
    pos.x += nx * correction * weight;
    pos.y += ny * correction * weight;
    pos.z += nz * correction * weight;
    tr.SetTranslation(pos);
    if (rb != nullptr && rb->GetBodyType() == RigidbodyBodyType3D::Dynamic) {
        Vector3 v = rb->GetVelocity();
        v.x += nx * correction * weight * 0.2F;
        v.y += ny * correction * weight * 0.2F;
        v.z += nz * correction * weight * 0.2F;
        rb->SetVelocity(v);
    }
}

}  // namespace

void SolveDistanceJoints2D(GameWorld& world, const float stiffnessScale) noexcept {
    world.ForEachActiveGameObject([&](GameObject* owner) {
        if (owner == nullptr) {
            return;
        }
        auto* joint = owner->GetComponent<DistanceJoint2DComponent>();
        if (joint == nullptr) {
            return;
        }
        GameObject* other = joint->GetConnectedBody();
        if (other == nullptr) {
            return;
        }
        auto* trA = owner->GetComponent<TransformComponent>();
        auto* trB = other->GetComponent<TransformComponent>();
        auto* rbA = owner->GetComponent<Rigidbody2DComponent>();
        auto* rbB = other->GetComponent<Rigidbody2DComponent>();
        if (trA == nullptr || trB == nullptr) {
            return;
        }
        const Vector2 a = WorldAnchor2D(*owner, joint->GetLocalAnchorA());
        const Vector2 b = WorldAnchor2D(*other, joint->GetLocalAnchorB());
        const Vector2 d = b - a;
        const float len = d.Length();
        if (len < 1.0e-6F) {
            return;
        }
        const float nx = d.x / len;
        const float ny = d.y / len;
        const float err = len - joint->GetRestLength();
        const float correction = err * joint->GetStiffness() * stiffnessScale;
        const float wA = (rbA != nullptr && rbA->GetBodyType() == RigidbodyBodyType2D::Dynamic) ? 1.0F : 0.0F;
        const float wB = (rbB != nullptr && rbB->GetBodyType() == RigidbodyBodyType2D::Dynamic) ? 1.0F : 0.0F;
        const float den = wA + wB;
        if (den <= 1.0e-8F) {
            return;
        }
        ApplyPositionDelta2D(*trA, rbA, nx * correction * wB / den, ny * correction * wB / den, 1.0F);
        ApplyPositionDelta2D(*trB, rbB, -nx * correction * wA / den, -ny * correction * wA / den, 1.0F);
    });
}

void SolveHingeJoints2D(GameWorld& world, const float stiffnessScale) noexcept {
    world.ForEachActiveGameObject([&](GameObject* owner) {
        if (owner == nullptr) {
            return;
        }
        auto* joint = owner->GetComponent<HingeJoint2DComponent>();
        if (joint == nullptr) {
            return;
        }
        GameObject* other = joint->GetConnectedBody();
        if (other == nullptr) {
            return;
        }
        auto* trA = owner->GetComponent<TransformComponent>();
        auto* trB = other->GetComponent<TransformComponent>();
        auto* rbA = owner->GetComponent<Rigidbody2DComponent>();
        auto* rbB = other->GetComponent<Rigidbody2DComponent>();
        if (trA == nullptr || trB == nullptr) {
            return;
        }
        const Vector2 a = WorldAnchor2D(*owner, joint->GetLocalAnchorA());
        const Vector2 b = WorldAnchor2D(*other, joint->GetLocalAnchorB());
        const Vector2 d = b - a;
        const float err = d.Length();
        if (err < 1.0e-6F) {
            return;
        }
        const float nx = d.x / err;
        const float ny = d.y / err;
        const float correction = err * joint->GetStiffness() * stiffnessScale;
        const float wA = (rbA != nullptr && rbA->GetBodyType() == RigidbodyBodyType2D::Dynamic) ? 1.0F : 0.0F;
        const float wB = (rbB != nullptr && rbB->GetBodyType() == RigidbodyBodyType2D::Dynamic) ? 1.0F : 0.0F;
        const float den = wA + wB;
        if (den <= 1.0e-8F) {
            return;
        }
        ApplyPositionDelta2D(*trA, rbA, nx * correction * wB / den, ny * correction * wB / den, 1.0F);
        ApplyPositionDelta2D(*trB, rbB, -nx * correction * wA / den, -ny * correction * wA / den, 1.0F);
    });
}

void SolveHingeJoints3D(GameWorld& world, const float stiffnessScale) noexcept {
    world.ForEachActiveGameObject([&](GameObject* owner) {
        if (owner == nullptr) {
            return;
        }
        auto* joint = owner->GetComponent<HingeJoint3DComponent>();
        if (joint == nullptr) {
            return;
        }
        GameObject* other = joint->GetConnectedBody();
        if (other == nullptr) {
            return;
        }
        auto* trA = owner->GetComponent<TransformComponent>();
        auto* trB = other->GetComponent<TransformComponent>();
        auto* rbA = owner->GetComponent<Rigidbody3DComponent>();
        auto* rbB = other->GetComponent<Rigidbody3DComponent>();
        if (trA == nullptr || trB == nullptr) {
            return;
        }
        const Vector3 a = WorldAnchor3D(*owner, joint->GetLocalAnchorA());
        const Vector3 b = WorldAnchor3D(*other, joint->GetLocalAnchorB());
        const Vector3 d = b - a;
        const float len = d.Length();
        if (len < 1.0e-6F) {
            return;
        }
        const float nx = d.x / len;
        const float ny = d.y / len;
        const float nz = d.z / len;
        const float correction = len * joint->GetStiffness() * stiffnessScale;
        const float wA = (rbA != nullptr && rbA->GetBodyType() == RigidbodyBodyType3D::Dynamic) ? rbA->GetInverseMass() : 0.0F;
        const float wB = (rbB != nullptr && rbB->GetBodyType() == RigidbodyBodyType3D::Dynamic) ? rbB->GetInverseMass() : 0.0F;
        const float den = wA + wB;
        if (den <= 1.0e-8F) {
            return;
        }
        ApplyPositionDelta3D(*trA, rbA, nx, ny, nz, correction * wB / den, 1.0F);
        ApplyPositionDelta3D(*trB, rbB, -nx, -ny, -nz, correction * wA / den, 1.0F);
    });
}

void SolveSpringJoints3D(GameWorld& world, const float deltaTimeSeconds) noexcept {
    if (deltaTimeSeconds <= 0.0F) {
        return;
    }
    world.ForEachActiveGameObject([&](GameObject* owner) {
        if (owner == nullptr) {
            return;
        }
        auto* joint = owner->GetComponent<SpringJoint3DComponent>();
        if (joint == nullptr) {
            return;
        }
        GameObject* other = joint->GetConnectedBody();
        if (other == nullptr) {
            return;
        }
        auto* trA = owner->GetComponent<TransformComponent>();
        auto* trB = other->GetComponent<TransformComponent>();
        auto* spA = owner->GetComponent<SphereCollider3DComponent>();
        auto* spB = other->GetComponent<SphereCollider3DComponent>();
        auto* rbA = owner->GetComponent<Rigidbody3DComponent>();
        auto* rbB = other->GetComponent<Rigidbody3DComponent>();
        if (trA == nullptr || trB == nullptr || spA == nullptr || spB == nullptr) {
            return;
        }
        Vector3 ca{};
        float ra = 0.0F;
        Vector3 cb{};
        float rb = 0.0F;
        ComputeSphereCollider3World(*owner, *spA, ca, ra);
        ComputeSphereCollider3World(*other, *spB, cb, rb);
        const Vector3 d = cb - ca;
        const float len = d.Length();
        if (len < 1.0e-6F) {
            return;
        }
        const float nx = d.x / len;
        const float ny = d.y / len;
        const float nz = d.z / len;
        float vn = 0.0F;
        if (rbA != nullptr && rbA->GetBodyType() == RigidbodyBodyType3D::Dynamic) {
            const Vector3 v = rbA->GetVelocity();
            vn += v.x * nx + v.y * ny + v.z * nz;
        }
        if (rbB != nullptr && rbB->GetBodyType() == RigidbodyBodyType3D::Dynamic) {
            const Vector3 v = rbB->GetVelocity();
            vn -= v.x * nx + v.y * ny + v.z * nz;
        }
        const float stretch = len - joint->GetRestLength();
        const float force = -joint->GetSpringStiffness() * stretch - joint->GetDamping() * vn;
        const float impulse = force * deltaTimeSeconds;
        if (rbA != nullptr && rbA->GetBodyType() == RigidbodyBodyType3D::Dynamic) {
            Vector3 v = rbA->GetVelocity();
            v.x += nx * impulse * rbA->GetInverseMass();
            v.y += ny * impulse * rbA->GetInverseMass();
            v.z += nz * impulse * rbA->GetInverseMass();
            rbA->SetVelocity(v);
        }
        if (rbB != nullptr && rbB->GetBodyType() == RigidbodyBodyType3D::Dynamic) {
            Vector3 v = rbB->GetVelocity();
            v.x -= nx * impulse * rbB->GetInverseMass();
            v.y -= ny * impulse * rbB->GetInverseMass();
            v.z -= nz * impulse * rbB->GetInverseMass();
            rbB->SetVelocity(v);
        }
    });
}

}  // namespace Spark
