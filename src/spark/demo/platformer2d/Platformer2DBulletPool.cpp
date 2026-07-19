#include "spark/demo/platformer2d/Platformer2DBulletPool.hpp"

#include "spark/demo/platformer2d/Platformer2DCombatMath.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

#include <algorithm>
#include <cmath>

namespace Spark::Platformer2D {

void BulletPool::DeactivateSlot(Slot& slot) noexcept
{
    slot.active = false;
    slot.vx = 0.0F;
    slot.vy = 0.0F;
    slot.age = 0.0F;
    slot.lifetime = 0.0F;
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
        if (texture.Get() != nullptr) {
            go->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
            Spark::SpriteComponent* spr = go->AddComponent<Spark::SpriteComponent>(
                    texture,
                    Spark::Vector4{1.0F, 1.0F, 1.0F, 0.0F},
                    Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                    sortOrderBase + bi);
            slots[static_cast<std::size_t>(bi)] = {false, go, tr, spr, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, {}};
        } else {
            slots[static_cast<std::size_t>(bi)] = {false, go, tr, nullptr, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, {}};
        }
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
        const BulletProfile& profile) noexcept
{
    CombatMath::NormalizeOrDefault(dirX, dirY, 1.0F, 0.0F, dirX, dirY);
    for (std::size_t bi = 0; bi < slots.GetSize(); ++bi) {
        Slot& slot = slots[bi];
        if (slot.active) {
            continue;
        }
        slot.active = true;
        slot.profile = profile;
        slot.vx = dirX * profile.speed;
        slot.vy = dirY * profile.speed;
        slot.age = 0.0F;
        slot.lifetime = profile.lifetime;
        slot.cx = originX + dirX * 0.35F;
        slot.cy = originY + dirY * 0.12F;
        if (slot.tr != nullptr) {
            slot.tr->SetTranslation({slot.cx, slot.cy, 0.06F});
            slot.tr->SetUniformScale(profile.drawScale);
            const float angleZ = std::atan2(slot.vy, slot.vx);
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
        const float cullMinX,
        const float cullMaxX,
        const float cullMinY,
        const float cullMaxY) noexcept
{
    for (std::size_t bi = 0; bi < slots.GetSize(); ++bi) {
        Slot& slot = slots[bi];
        if (!slot.active) {
            continue;
        }
        slot.age += deltaSeconds;
        if (slot.age >= slot.lifetime) {
            DeactivateSlot(slot);
            continue;
        }
        slot.cx += slot.vx * deltaSeconds;
        slot.cy += slot.vy * deltaSeconds;
        if (slot.tr != nullptr) {
            slot.tr->SetTranslation({slot.cx, slot.cy, 0.06F});
        }
        if (slot.spr != nullptr) {
            const float pulse = 0.78F + 0.22F * std::sin(slot.age * 18.0F);
            const float fade = 1.0F -
                    std::clamp((slot.age - slot.lifetime * 0.72F) / (slot.lifetime * 0.28F), 0.0F, 1.0F);
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
