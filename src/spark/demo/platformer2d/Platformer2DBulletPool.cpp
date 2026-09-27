#include "spark/demo/platformer2d/Platformer2DBulletPool.hpp"

#include "spark/demo/platformer2d/Platformer2DCombatMath.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

#include <algorithm>
#include <cmath>

namespace Spark::Platformer2D {

void BulletPool::ApplyProfileToProjectile(
        Spark::Projectile2DComponent& projectile,
        const BulletProfile& profile) noexcept {
    projectile.SetShape(profile.projectileShape);
    if (profile.projectileShape == Spark::Projectile2DShape::Box) {
        projectile.SetHalfExtents({profile.halfW, profile.halfH});
    } else {
        projectile.SetRadius(std::max(profile.halfW, profile.halfH));
    }
    projectile.SetDamage(profile.damage);
    projectile.SetTargetFilter(profile.targetFilter);
    projectile.SetBlockOnSolidHit(profile.blockOnSolidHit);
    projectile.SetLifetimeSeconds(profile.lifetime);
    projectile.SetDestroyOwnerOnDeactivate(false);
    projectile.SetDeactivateOnFirstHit(true);
}

void BulletPool::WireProjectileCallbacks(const std::size_t slotIndex) noexcept {
    if (slotIndex >= slots.GetSize() || slots[slotIndex].projectile == nullptr) {
        return;
    }
    slots[slotIndex].projectile->SetOnHit(
            [this, slotIndex](Spark::GameObject& /*projectileOwner*/, Spark::GameObject& victim, float applied) {
                if (onTargetHit) {
                    onTargetHit(slots[slotIndex], victim, applied);
                }
            });
}

void BulletPool::DeactivateSlot(Slot& slot) noexcept
{
    if (slot.projectile != nullptr) {
        slot.projectile->Deactivate();
    }
    slot.active = false;
    slot.cx = 0.0F;
    slot.cy = 0.0F;
    slot.age = 0.0F;
    if (slot.tr != nullptr) {
        slot.tr->SetTranslation({-120.0F, -120.0F, 0.0F});
        slot.tr->SetRotation(Spark::Quaternion::Identity);
    }
    if (slot.spr != nullptr) {
        const Spark::Vector4 tint = slot.spr->GetTint();
        slot.spr->SetTint({tint.x, tint.y, tint.z, 0.0F});
    }
}

void BulletPool::Initialize(
        Spark::GameWorld& world,
        const Spark::SharedPtr<Spark::Texture2D>& texture,
        const int capacity,
        const int sortOrderBase,
        Spark::Utf8String debugNamePrefix,
        Spark::DemoRootCollection& roots)
{
    Shutdown(world);
    slots.Clear();
    slots.Resize(static_cast<std::size_t>(capacity));
    for (int bi = 0; bi < capacity; ++bi) {
        Spark::GameObject* go = world.CreateGameObject();
        go->GetName() = debugNamePrefix;
        Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
        tr->SetTranslation({-120.0F, -120.0F, 0.0F});
        Spark::SpriteComponent* spr = nullptr;
        if (texture.Get() != nullptr) {
            go->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
            spr = go->AddComponent<Spark::SpriteComponent>(
                    texture,
                    Spark::Vector4{1.0F, 1.0F, 1.0F, 0.0F},
                    Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                    sortOrderBase + bi);
        }
        Spark::Projectile2DComponent* projectile = go->AddComponent<Spark::Projectile2DComponent>();
        slots[static_cast<std::size_t>(bi)] = Slot{false, go, tr, spr, projectile, 0.0F, 0.0F, 0.0F, {}};
        WireProjectileCallbacks(static_cast<std::size_t>(bi));
        roots.Track(go);
    }
}

void BulletPool::Shutdown(Spark::GameWorld& /*world*/) noexcept
{
    slots.Clear();
}

bool BulletPool::TrySpawn(
        const float originX,
        const float originY,
        float dirX,
        float dirY,
        Spark::GameObject* instigator,
        const BulletProfile& profile) noexcept
{
    CombatMath::NormalizeOrDefault(dirX, dirY, 1.0F, 0.0F, dirX, dirY);
    for (std::size_t bi = 0; bi < slots.GetSize(); ++bi) {
        Slot& slot = slots[bi];
        if (slot.active || slot.projectile == nullptr) {
            continue;
        }
        slot.profile = profile;
        ApplyProfileToProjectile(*slot.projectile, profile);
        slot.projectile->SetInstigator(instigator);

        const float spawnX = originX + dirX * 0.35F;
        const float spawnY = originY + dirY * 0.12F;
        slot.projectile->Activate({spawnX, spawnY}, {dirX * profile.speed, dirY * profile.speed}, instigator);
        slot.active = slot.projectile->IsActive();
        if (!slot.active) {
            continue;
        }

        slot.cx = spawnX;
        slot.cy = spawnY;
        slot.age = 0.0F;
        if (slot.tr != nullptr) {
            slot.tr->SetTranslation({spawnX, spawnY, 0.06F});
            slot.tr->SetUniformScale(profile.drawScale);
            const float angleZ = std::atan2(dirY * profile.speed, dirX * profile.speed);
            slot.tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitZ, angleZ));
        }
        if (slot.spr != nullptr) {
            slot.spr->SetTint(profile.baseTint);
        }
        return true;
    }
    return false;
}

void BulletPool::Tick(
        const float deltaSeconds,
        Spark::IEngineContext& context,
        const float cullMinX,
        const float cullMaxX,
        const float cullMinY,
        const float cullMaxY) noexcept
{
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = deltaSeconds;

    for (std::size_t bi = 0; bi < slots.GetSize(); ++bi) {
        Slot& slot = slots[bi];
        if (!slot.active || slot.projectile == nullptr || slot.go == nullptr) {
            continue;
        }

        slot.projectile->OnUpdate(timing, *slot.go, context);
        if (!slot.projectile->IsActive()) {
            DeactivateSlot(slot);
            continue;
        }

        slot.age += deltaSeconds;
        if (slot.tr != nullptr) {
            const Spark::Vector3 pos = slot.tr->GetLocalTransform().translation;
            slot.cx = pos.x;
            slot.cy = pos.y;
        }

        if (slot.spr != nullptr) {
            const float pulse = 0.78F + 0.22F * std::sin(slot.age * 18.0F);
            const float fade = 1.0F -
                    std::clamp(
                            (slot.age - slot.profile.lifetime * 0.72F) / (slot.profile.lifetime * 0.28F),
                            0.0F,
                            1.0F);
            const Spark::Vector4 base = slot.profile.baseTint;
            slot.spr->SetTint(
                    {base.x,
                     base.y + 0.08F * pulse,
                     base.z,
                     base.w * (0.55F + 0.35F * pulse) * fade});
        }

        if (slot.cx < cullMinX || slot.cx > cullMaxX || slot.cy < cullMinY || slot.cy > cullMaxY) {
            DeactivateSlot(slot);
        }
    }
}

void BulletPool::DeactivateAll() noexcept
{
    for (std::size_t bi = 0; bi < slots.GetSize(); ++bi) {
        DeactivateSlot(slots[bi]);
    }
}

}  // namespace Spark::Platformer2D
