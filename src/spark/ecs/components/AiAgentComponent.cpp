#include "spark/ecs/components/AiAgentComponent.hpp"

#include "spark/ai/goap/GoapPlanner.hpp"
#include "spark/ai/steering/SteeringBehaviors.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/engine/IEngineContext.hpp"

#include <cmath>

namespace Spark {

void AiAgentComponent::SubsystemTick(const FrameTiming& timing, GameObject& owner, IEngineContext& context) {
    (void)context;
    if (!enabled) {
        return;
    }
    const float dt = timing.deltaTimeSeconds;

    if (fsmEnabled && fsm) {
        fsm->Tick(timing, blackboard);
    }

    if (goapEnabled && !goapActions.IsEmpty()) {
        if (goapPlan.IsEmpty()) {
            [[maybe_unused]] const bool planned = GoapPlanner::Plan(
                    goapWorldBits, goapGoalMask, goapGoalValue, goapActions, goapPlan);
        }
        if (!goapPlan.IsEmpty()) {
            const std::uint32_t aix = goapPlan[0];
            if (aix < goapActions.GetSize()) {
                const GoapActionSpec& a = goapActions[aix];
                goapWorldBits = (goapWorldBits | a.effectSetMask) & ~a.effectClearMask;
            }
            goapPlan.RemoveAt(0);
        }
    }

    if (fuzzyEnabled && fuzzyModule) {
        fuzzyModule->Evaluate(blackboard);
    }

    TransformComponent* tr = owner.GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }

    if (pathIndex >= 0 && static_cast<std::size_t>(pathIndex) < pathWorldXZ.GetSize()) {
        const Vector2& wp = pathWorldXZ[static_cast<std::size_t>(pathIndex)];
        blackboard.SetFloat(0, wp.x);
        blackboard.SetFloat(1, wp.y);
        const Vector3 p = tr->GetLocalTransform().translation;
        const Vector2 pos{p.x, p.z};
        if ((pos - wp).LengthSquared() < 0.04F) {
            ++pathIndex;
        }
    }

    SteeringSeek seek(1.0F);
    SteeringComposer comp;
    comp.AddBehavior(seek, 1.0F);

    Vector2 pos{0.0F, 0.0F};
    Vector2 curVel{0.0F, 0.0F};
    if (plane == AiSteeringPlane::XzWorld) {
        const Vector3 p = tr->GetLocalTransform().translation;
        pos = Vector2{p.x, p.z};
        if (Rigidbody3DComponent* r3 = owner.GetComponent<Rigidbody3DComponent>()) {
            if (r3->GetBodyType() == RigidbodyBodyType3D::Dynamic) {
                const Vector3 v = r3->GetVelocity();
                curVel = Vector2{v.x, v.z};
            }
        }
    } else {
        const Vector3 p = tr->GetLocalTransform().translation;
        pos = Vector2{p.x, p.y};
        if (Rigidbody2DComponent* r2 = owner.GetComponent<Rigidbody2DComponent>()) {
            if (r2->GetBodyType() == RigidbodyBodyType2D::Dynamic) {
                curVel = r2->GetVelocity();
            }
        }
    }

    Vector2 velDesired = comp.Compose(pos, curVel, blackboard);
    const float cap = std::max(0.0F, maxSpeed);
    if (velDesired.LengthSquared() > 1.0e-8F && velDesired.Length() > cap) {
        velDesired = velDesired.Normalized() * cap;
    }

    if (plane == AiSteeringPlane::XyRigidbody2D) {
        if (Rigidbody2DComponent* rb = owner.GetComponent<Rigidbody2DComponent>()) {
            if (rb->GetBodyType() == RigidbodyBodyType2D::Dynamic) {
                rb->SetVelocity(velDesired);
            }
        }
        return;
    }

    if (Rigidbody3DComponent* r3 = owner.GetComponent<Rigidbody3DComponent>()) {
        if (r3->GetBodyType() == RigidbodyBodyType3D::Dynamic) {
            Vector3 v = r3->GetVelocity();
            v.x = velDesired.x;
            v.z = velDesired.y;
            r3->SetVelocity(v);
            return;
        }
    }

    Vector3 p = tr->GetLocalTransform().translation;
    p.x += velDesired.x * dt;
    p.z += velDesired.y * dt;
    tr->SetTranslation(p);
}

}  // namespace Spark
