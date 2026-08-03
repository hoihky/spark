#include "spark/physics/simulation/TriggerDispatcher2D.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"
#include "spark/physics/colliders/Collider2D.hpp"

namespace Spark {

namespace {

void EmitTriggerOverlap2D(
        GameObject& trigger,
        GameObject& other,
        const std::uint32_t staticIndex) noexcept {
    SignalPayload p{};
    p.ptr = &other;
    p.a = other.GetId();
    p.b = staticIndex;
    trigger.EmitSignal(SignalId::Physics2DTriggerOverlap, p, nullptr);
}

}  // namespace

void TriggerDispatcher2D::ReportStaticDynamic(
        GameObject& dynamic,
        const Collider2D& col,
        const std::uint32_t staticIndex,
        const bool dynamicColliderIsTrigger) noexcept {
    if (col.IsTrigger() && col.GetOwner() != nullptr) {
        EmitTriggerOverlap2D(*col.GetOwner(), dynamic, staticIndex);
    }
    if (dynamicColliderIsTrigger && col.GetOwner() != nullptr) {
        EmitTriggerOverlap2D(dynamic, *col.GetOwner(), staticIndex);
    }
}

void TriggerDispatcher2D::ReportDynamicDynamic(
        GameObject& a,
        GameObject& b,
        const bool aTrigger,
        const bool bTrigger) noexcept {
    if (aTrigger) {
        EmitTriggerOverlap2D(a, b, kPhysics2DTriggerOverlapNoStaticIndex);
    }
    if (bTrigger) {
        EmitTriggerOverlap2D(b, a, kPhysics2DTriggerOverlapNoStaticIndex);
    }
}

}  // namespace Spark
