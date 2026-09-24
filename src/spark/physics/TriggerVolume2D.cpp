#include "spark/physics/TriggerVolume2D.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CharacterController2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/2d/TriggerVolume2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] float MaxMatrixScale2D(const Matrix4& wm) noexcept {
    const float sx = std::sqrt(wm.m[0] * wm.m[0] + wm.m[1] * wm.m[1] + wm.m[2] * wm.m[2]);
    const float sy = std::sqrt(wm.m[4] * wm.m[4] + wm.m[5] * wm.m[5] + wm.m[6] * wm.m[6]);
    return std::max(sx, sy);
}

[[nodiscard]] bool ContainsId(const Array<std::uint64_t>& ids, const std::uint64_t id) noexcept {
    for (std::size_t i = 0; i < ids.GetSize(); ++i) {
        if (ids[i] == id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool PassesTagFilter(const TriggerVolume2DComponent& volume, const GameObject& probeObject) noexcept {
    const char* required = volume.GetFilterTag();
    if (required == nullptr || required[0] == '\0') {
        return true;
    }
    return std::strcmp(probeObject.GetTag().CStr(), required) == 0;
}

[[nodiscard]] bool AabbOverlapsCircle(
        const CollisionAabb2& box,
        const float cx,
        const float cy,
        const float radius) noexcept {
    const float qx = std::clamp(cx, box.minX, box.maxX);
    const float qy = std::clamp(cy, box.minY, box.maxY);
    const float dx = cx - qx;
    const float dy = cy - qy;
    return dx * dx + dy * dy <= radius * radius + 1.0e-8F;
}

struct TriggerVolumeEntry2D {
    GameObject* owner = nullptr;
    TriggerVolume2DComponent* volume = nullptr;
    TriggerVolume2DWorld world{};
};

struct TriggerProbeEntry2D {
    GameObject* owner = nullptr;
    TriggerProbe2DWorld probe{};
};

}  // namespace

void BuildTriggerVolume2DWorld(
        GameObject& owner,
        const TriggerVolume2DComponent& volume,
        TriggerVolume2DWorld& outWorld) noexcept {
    outWorld.shape = volume.GetShape();
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector2 off = volume.GetOffset();
    const float scale = MaxMatrixScale2D(wm);

    if (outWorld.shape == TriggerVolume2DShape::Box) {
        const Vector2 he = volume.GetHalfExtents();
        const float x0 = off.x - he.x;
        const float y0 = off.y - he.y;
        const float x1 = off.x + he.x;
        const float y1 = off.y + he.y;
        const Vector4 corners[4] = {
                wm * Vector4(x0, y0, 0.0F, 1.0F),
                wm * Vector4(x1, y0, 0.0F, 1.0F),
                wm * Vector4(x1, y1, 0.0F, 1.0F),
                wm * Vector4(x0, y1, 0.0F, 1.0F),
        };
        float minX = corners[0].x;
        float maxX = corners[0].x;
        float minY = corners[0].y;
        float maxY = corners[0].y;
        for (int i = 1; i < 4; ++i) {
            minX = std::min(minX, corners[i].x);
            maxX = std::max(maxX, corners[i].x);
            minY = std::min(minY, corners[i].y);
            maxY = std::max(maxY, corners[i].y);
        }
        outWorld.box.minX = minX;
        outWorld.box.maxX = maxX;
        outWorld.box.minY = minY;
        outWorld.box.maxY = maxY;
        outWorld.bounds = outWorld.box;
        return;
    }

    const Vector4 center4 = wm * Vector4(off.x, off.y, 0.0F, 1.0F);
    outWorld.circleCx = center4.x;
    outWorld.circleCy = center4.y;
    outWorld.circleR = volume.GetRadius() * scale;
    outWorld.bounds.minX = outWorld.circleCx - outWorld.circleR;
    outWorld.bounds.maxX = outWorld.circleCx + outWorld.circleR;
    outWorld.bounds.minY = outWorld.circleCy - outWorld.circleR;
    outWorld.bounds.maxY = outWorld.circleCy + outWorld.circleR;
}

bool TriggerVolume2DOverlapsProbe(
        const TriggerVolume2DWorld& volume,
        const TriggerProbe2DWorld& probe) noexcept {
    if (!CollisionAabb2Overlaps(volume.bounds, probe.box)) {
        return false;
    }

    if (volume.shape == TriggerVolume2DShape::Box) {
        if (probe.useCircle) {
            return AabbOverlapsCircle(volume.box, probe.circleCx, probe.circleCy, probe.circleR);
        }
        return CollisionAabb2Overlaps(volume.box, probe.box);
    }

    if (probe.useCircle) {
        return CollisionCirclesOverlap(
                volume.circleCx, volume.circleCy, volume.circleR, probe.circleCx, probe.circleCy, probe.circleR);
    }
    return AabbOverlapsCircle(probe.box, volume.circleCx, volume.circleCy, volume.circleR);
}

bool TryBuildTriggerProbe2DFromObject(
        GameObject& object,
        const TriggerVolume2DSettings& settings,
        TriggerProbe2DWorld& outProbe) noexcept {
    auto buildFromColliders = [&]() -> bool {
        if (auto* circle = object.GetComponent<CircleCollider2DComponent>()) {
            ComputeCircleCollider2World(object, *circle, outProbe.circleCx, outProbe.circleCy, outProbe.circleR);
            outProbe.useCircle = true;
            outProbe.box.minX = outProbe.circleCx - outProbe.circleR;
            outProbe.box.maxX = outProbe.circleCx + outProbe.circleR;
            outProbe.box.minY = outProbe.circleCy - outProbe.circleR;
            outProbe.box.maxY = outProbe.circleCy + outProbe.circleR;
            return true;
        }
        if (auto* box = object.GetComponent<BoxCollider2DComponent>()) {
            ComputeBoxCollider2WorldAabb(object, *box, outProbe.box);
            outProbe.useCircle = false;
            return true;
        }
        return false;
    };

    auto* rb = object.GetComponent<Rigidbody2DComponent>();
    const bool isDynamicRb = rb != nullptr && rb->GetBodyType() == RigidbodyBodyType2D::Dynamic;
    const bool hasController = object.GetComponent<CharacterController2DComponent>() != nullptr;

    if (settings.includeDynamicRigidbodies && isDynamicRb && buildFromColliders()) {
        return true;
    }
    if (settings.includeColliderWithoutRigidbody && !isDynamicRb && buildFromColliders()) {
        return true;
    }
    if (settings.includeCharacterControllers && hasController && buildFromColliders()) {
        return true;
    }
    return false;
}

void TriggerVolumeWorld2D::Simulate(GameWorld& world, const FrameTiming& timing) {
    (void)timing;

    Array<TriggerVolumeEntry2D> volumes;
    Array<TriggerProbeEntry2D> probes;

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        if (auto* volume = object->GetComponent<TriggerVolume2DComponent>()) {
            if (object->GetComponent<TransformComponent>() == nullptr) {
                return;
            }
            TriggerVolumeEntry2D entry{};
            entry.owner = object;
            entry.volume = volume;
            BuildTriggerVolume2DWorld(*object, *volume, entry.world);
            volumes.PushBack(entry);
        }
    });

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        TriggerProbe2DWorld probe{};
        if (!TryBuildTriggerProbe2DFromObject(*object, settings, probe)) {
            return;
        }
        TriggerProbeEntry2D entry{};
        entry.owner = object;
        entry.probe = probe;
        probes.PushBack(entry);
    });

    for (std::size_t vi = 0; vi < volumes.GetSize(); ++vi) {
        TriggerVolumeEntry2D& volumeEntry = volumes[vi];
        TriggerVolume2DComponent& volume = *volumeEntry.volume;
        const Array<std::uint64_t> previous = volume.overlappingIds;
        Array<std::uint64_t> current;
        current.Clear();

        for (std::size_t pi = 0; pi < probes.GetSize(); ++pi) {
            const TriggerProbeEntry2D& probeEntry = probes[pi];
            if (probeEntry.owner == volumeEntry.owner) {
                continue;
            }
            if (!PassesTagFilter(volume, *probeEntry.owner)) {
                continue;
            }
            if (!TriggerVolume2DOverlapsProbe(volumeEntry.world, probeEntry.probe)) {
                continue;
            }
            const std::uint64_t probeId = probeEntry.owner->GetId();
            if (!ContainsId(current, probeId)) {
                current.PushBack(probeId);
            }
        }

        for (std::size_t i = 0; i < current.GetSize(); ++i) {
            const std::uint64_t id = current[i];
            GameObject* other = world.FindGameObjectById(id);
            if (other == nullptr) {
                continue;
            }
            if (ContainsId(previous, id)) {
                volume.NotifyStay(*other);
            } else {
                volume.NotifyEnter(*other);
            }
        }

        for (std::size_t i = 0; i < previous.GetSize(); ++i) {
            const std::uint64_t id = previous[i];
            if (ContainsId(current, id)) {
                continue;
            }
            GameObject* other = world.FindGameObjectById(id);
            if (other != nullptr) {
                volume.NotifyExit(*other);
            }
        }

        volume.SetOverlappingIds(MoveTemp(current));
    }
}

void SimulateTriggerVolumes2D(
        GameWorld& world,
        const FrameTiming& timing,
        const TriggerVolume2DSettings& settings) {
    TriggerVolumeWorld2D triggerWorld(settings);
    triggerWorld.Simulate(world, timing);
}

}  // namespace Spark
