#include "spark/gameplay/Combat2D.hpp"

#include "spark/ecs/components/animation/Sprite2DCharacterAnimFsmComponent.hpp"
#include "spark/ecs/components/physics/2d/Hurtbox2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/GameplayDamageApplier.hpp"
#include "spark/gameplay/GameplayDamageDeal.hpp"
#include "spark/gameplay/GameplayTeamPolicy.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

namespace {

class Hurtbox2DNotifier final : public GameplayDamageApplier::IHurtboxNotifier {
public:
    explicit Hurtbox2DNotifier(Hurtbox2DComponent* hurtboxIn) noexcept : hurtbox(hurtboxIn) {}
    void NotifyDamageReceived() noexcept override {
        if (hurtbox != nullptr) {
            hurtbox->NotifyDamageReceived();
        }
    }

private:
    Hurtbox2DComponent* hurtbox = nullptr;
};

const GameplayTeamPolicy& DefaultTeamPolicy() noexcept {
    static const GameplayTeamPolicy policy{};
    return policy;
}

}  // namespace

float Combat2DTarget::ApplyDamage(GameObject* instigator, const float rawDamage) noexcept {
    if (target == nullptr || rawDamage <= 0.0F || !target->IsActiveInHierarchy()) {
        return 0.0F;
    }

    Hurtbox2DComponent* hurtbox = target->GetComponent<Hurtbox2DComponent>();
    if (hurtbox != nullptr) {
        if (!hurtbox->IsEnabled() || hurtbox->IsInvulnerable()) {
            return 0.0F;
        }
    }

    GameplayDamageDeal deal{*target};
    deal.SetInstigator(instigator);
    deal.SetTeamPolicy(teamPolicy != nullptr ? teamPolicy : &DefaultTeamPolicy());
    Hurtbox2DNotifier notifier{hurtbox};
    deal.SetHurtboxNotifier(hurtbox != nullptr ? &notifier : nullptr);
    return deal.Apply(rawDamage);
}

void Combat2DTarget::RequestHurtReaction() noexcept {
    if (target == nullptr) {
        return;
    }
    if (Sprite2DCharacterAnimFsmComponent* fsm = Sprite2DCharacterAnimFsmComponent::FindInHierarchy(*target)) {
        fsm->RequestHurt();
    }
}

void CombatHurtboxWorldQuery2D::AppendUniqueHurtbox_(
        GameObject* candidate,
        GameObject* ignoreObject,
        Array<CombatHurtboxOverlap2D>& outOverlaps,
        Array<GameObject*>& seenOwners) noexcept {
    if (candidate == nullptr || candidate == ignoreObject || !candidate->IsActiveInHierarchy()) {
        return;
    }
    for (std::size_t i = 0; i < seenOwners.GetSize(); ++i) {
        if (seenOwners[i] == candidate) {
            return;
        }
    }
    Hurtbox2DComponent* hurtbox = candidate->GetComponent<Hurtbox2DComponent>();
    if (hurtbox == nullptr || !hurtbox->IsEnabled()) {
        return;
    }
    seenOwners.PushBack(candidate);
    outOverlaps.PushBack(CombatHurtboxOverlap2D{candidate, hurtbox});
}

void CombatHurtboxWorldQuery2D::CollectCircleOverlaps(
        GameWorld& world,
        const float centerX,
        const float centerY,
        const float radius,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignoreObject,
        Array<CombatHurtboxOverlap2D>& outOverlaps) noexcept {
    outOverlaps.Clear();
    if (radius <= 0.0F) {
        return;
    }

    Array<PhysicsQueryHit2D> staticHits{};
    Array<PhysicsQueryHitDynamic2D> dynamicHits{};
    QueryOverlapCircleWorld2D(world, centerX, centerY, radius, filter, staticHits);
    QueryOverlapCircleDynamics2D(world, centerX, centerY, radius, filter, ignoreObject, dynamicHits);

    Array<GameObject*> seenOwners{};
    for (std::size_t i = 0; i < staticHits.GetSize(); ++i) {
        AppendUniqueHurtbox_(staticHits[i].owner, ignoreObject, outOverlaps, seenOwners);
    }
    for (std::size_t i = 0; i < dynamicHits.GetSize(); ++i) {
        AppendUniqueHurtbox_(dynamicHits[i].owner, ignoreObject, outOverlaps, seenOwners);
    }
}

void CombatHurtboxWorldQuery2D::CollectArcOverlaps(
        GameWorld& world,
        const float originX,
        const float originY,
        const float radius,
        const float dirX,
        const float dirY,
        const float halfAngleRadians,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignoreObject,
        Array<CombatHurtboxOverlap2D>& outOverlaps) noexcept {
    outOverlaps.Clear();
    if (radius <= 0.0F) {
        return;
    }

    Array<PhysicsQueryHit2D> staticHits{};
    Array<PhysicsQueryHitDynamic2D> dynamicHits{};
    QueryOverlapArcWorldStatics2D(
            world, originX, originY, radius, dirX, dirY, halfAngleRadians, filter, staticHits);
    QueryOverlapArcWorldDynamics2D(
            world, originX, originY, radius, dirX, dirY, halfAngleRadians, filter, ignoreObject, dynamicHits);

    Array<GameObject*> seenOwners{};
    for (std::size_t i = 0; i < staticHits.GetSize(); ++i) {
        AppendUniqueHurtbox_(staticHits[i].owner, ignoreObject, outOverlaps, seenOwners);
    }
    for (std::size_t i = 0; i < dynamicHits.GetSize(); ++i) {
        AppendUniqueHurtbox_(dynamicHits[i].owner, ignoreObject, outOverlaps, seenOwners);
    }
}

void CombatHurtboxWorldQuery2D::CollectAabbOverlaps(
        GameWorld& world,
        const CollisionAabb2& worldAabb,
        const float dynamicQueryRadius,
        const PhysicsQueryFilter2D& filter,
        GameObject* ignoreObject,
        Array<CombatHurtboxOverlap2D>& outOverlaps) noexcept {
    outOverlaps.Clear();

    Array<PhysicsQueryHit2D> staticHits{};
    Array<PhysicsQueryHitDynamic2D> dynamicHits{};
    QueryOverlapAabbWorld2D(world, worldAabb, filter, staticHits);
    if (dynamicQueryRadius > 0.0F) {
        const float centerX = (worldAabb.minX + worldAabb.maxX) * 0.5F;
        const float centerY = (worldAabb.minY + worldAabb.maxY) * 0.5F;
        QueryOverlapCircleDynamics2D(
                world, centerX, centerY, dynamicQueryRadius, filter, ignoreObject, dynamicHits);
    }

    Array<GameObject*> seenOwners{};
    for (std::size_t i = 0; i < staticHits.GetSize(); ++i) {
        AppendUniqueHurtbox_(staticHits[i].owner, ignoreObject, outOverlaps, seenOwners);
    }
    for (std::size_t i = 0; i < dynamicHits.GetSize(); ++i) {
        AppendUniqueHurtbox_(dynamicHits[i].owner, ignoreObject, outOverlaps, seenOwners);
    }
}

}  // namespace Spark
