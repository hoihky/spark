#include "spark/physics/simulation/JointSolver3D.hpp"

#include "spark/ecs/components/physics/3d/DistanceJoint3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/PhysicsJoints.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

void SolveDistanceJoints(GameWorld& world, const float stiffnessScale) noexcept {
    world.ForEachActiveGameObject([&](GameObject* owner) {
        if (owner == nullptr) {
            return;
        }
        auto* joint = owner->GetComponent<DistanceJoint3DComponent>();
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

        Vector3 d = cb - ca;
        const float len = d.Length();
        if (len < 1.0e-6F) {
            return;
        }
        const float nx = d.x / len;
        const float ny = d.y / len;
        const float nz = d.z / len;

        const float wA = (rbA != nullptr && rbA->GetBodyType() == RigidbodyBodyType3D::Dynamic) ? rbA->GetInverseMass() : 0.0F;
        const float wB = (rbB != nullptr && rbB->GetBodyType() == RigidbodyBodyType3D::Dynamic) ? rbB->GetInverseMass() : 0.0F;

        const float err = len - joint->GetRestLength();
        const float k = joint->GetStiffness() * stiffnessScale;
        const float correction = err * k;

        if (wA > 1.0e-8F && wB > 1.0e-8F) {
            const float den = wA + wB;
            const float uA = nx * correction * wB / den;
            const float uAy = ny * correction * wB / den;
            const float uAz = nz * correction * wB / den;
            const float uB = -nx * correction * wA / den;
            const float uBy = -ny * correction * wA / den;
            const float uBz = -nz * correction * wA / den;
            Vector3 pa = trA->GetLocalTransform().translation;
            pa.x += uA;
            pa.y += uAy;
            pa.z += uAz;
            trA->SetTranslation(pa);
            Vector3 pb = trB->GetLocalTransform().translation;
            pb.x += uB;
            pb.y += uBy;
            pb.z += uBz;
            trB->SetTranslation(pb);
        } else if (wA > 1.0e-8F) {
            Vector3 pa = trA->GetLocalTransform().translation;
            pa.x += nx * correction;
            pa.y += ny * correction;
            pa.z += nz * correction;
            trA->SetTranslation(pa);
        } else if (wB > 1.0e-8F) {
            Vector3 pb = trB->GetLocalTransform().translation;
            pb.x -= nx * correction;
            pb.y -= ny * correction;
            pb.z -= nz * correction;
            trB->SetTranslation(pb);
        }
    });
}

}  // namespace

void JointSolver3D::SolveSubstep(GameWorld& world, const float substepDt, const PhysicsWorld3DSettings& settings) noexcept {
    const int jIters = std::max(0, settings.jointIterations);
    for (int j = 0; j < jIters; ++j) {
        const float scale = 1.0F / static_cast<float>(std::max(1, jIters));
        SolveDistanceJoints(world, scale);
        SolveHingeJoints3D(world, scale);
    }
    SolveSpringJoints3D(world, substepDt);
}

}  // namespace Spark
