#include "spark/physics/TriggerVolume3D.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] Vector3 Hp3(const Vector4& p) noexcept {
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

[[nodiscard]] Vector3 LocalCapsuleAxis(const CapsuleDirection3D direction) noexcept {
    switch (direction) {
        case CapsuleDirection3D::X:
            return {1.0F, 0.0F, 0.0F};
        case CapsuleDirection3D::Z:
            return {0.0F, 0.0F, 1.0F};
        case CapsuleDirection3D::Y:
        default:
            return {0.0F, 1.0F, 0.0F};
    }
}

[[nodiscard]] float MaxMatrixScale(const Matrix4& wm) noexcept {
    const float sx = std::sqrt(wm.m[0] * wm.m[0] + wm.m[1] * wm.m[1] + wm.m[2] * wm.m[2]);
    const float sy = std::sqrt(wm.m[4] * wm.m[4] + wm.m[5] * wm.m[5] + wm.m[6] * wm.m[6]);
    const float sz = std::sqrt(wm.m[8] * wm.m[8] + wm.m[9] * wm.m[9] + wm.m[10] * wm.m[10]);
    return std::max({sx, sy, sz});
}

void BuildCapsuleBounds(const CollisionCapsule3& capsule, CollisionAabb3& outBounds) noexcept {
    outBounds.minX = std::min(capsule.pointA.x, capsule.pointB.x) - capsule.radius;
    outBounds.minY = std::min(capsule.pointA.y, capsule.pointB.y) - capsule.radius;
    outBounds.minZ = std::min(capsule.pointA.z, capsule.pointB.z) - capsule.radius;
    outBounds.maxX = std::max(capsule.pointA.x, capsule.pointB.x) + capsule.radius;
    outBounds.maxY = std::max(capsule.pointA.y, capsule.pointB.y) + capsule.radius;
    outBounds.maxZ = std::max(capsule.pointA.z, capsule.pointB.z) + capsule.radius;
}

void BuildSphereBounds(const Vector3& center, const float radius, CollisionAabb3& outBounds) noexcept {
    outBounds.minX = center.x - radius;
    outBounds.minY = center.y - radius;
    outBounds.minZ = center.z - radius;
    outBounds.maxX = center.x + radius;
    outBounds.maxY = center.y + radius;
    outBounds.maxZ = center.z + radius;
}

[[nodiscard]] bool ContainsId(const Array<std::uint64_t>& ids, const std::uint64_t id) noexcept {
    for (std::size_t i = 0; i < ids.GetSize(); ++i) {
        if (ids[i] == id) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool PassesTagFilter(const TriggerVolume3DComponent& volume, const GameObject& probeObject) noexcept {
    const char* required = volume.GetFilterTag();
    if (required == nullptr || required[0] == '\0') {
        return true;
    }
    return std::strcmp(probeObject.GetTag().CStr(), required) == 0;
}

[[nodiscard]] bool SphereSphereOverlap(
        const Vector3& aCenter,
        const float aRadius,
        const Vector3& bCenter,
        const float bRadius) noexcept {
    const float dx = bCenter.x - aCenter.x;
    const float dy = bCenter.y - aCenter.y;
    const float dz = bCenter.z - aCenter.z;
    const float rr = aRadius + bRadius;
    return dx * dx + dy * dy + dz * dz <= rr * rr + 1.0e-8F;
}

struct TriggerVolumeEntry {
    GameObject* owner = nullptr;
    TriggerVolume3DComponent* volume = nullptr;
    TriggerVolume3DWorld world{};
};

struct TriggerProbeEntry {
    GameObject* owner = nullptr;
    DynamicCollider3DSim sim{};
};

}  // namespace

void BuildTriggerVolume3DWorld(
        GameObject& owner,
        const TriggerVolume3DComponent& volume,
        TriggerVolume3DWorld& outWorld) noexcept {
    outWorld.shape = volume.GetShape();
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector3 off = volume.GetOffset();

    if (outWorld.shape == TriggerVolume3DShape::Box) {
        const Vector3 he = volume.GetHalfExtents();
        const float x0 = off.x - he.x;
        const float y0 = off.y - he.y;
        const float z0 = off.z - he.z;
        const float x1 = off.x + he.x;
        const float y1 = off.y + he.y;
        const float z1 = off.z + he.z;
        const Vector4 corners[8] = {
                wm * Vector4(x0, y0, z0, 1.0F),
                wm * Vector4(x1, y0, z0, 1.0F),
                wm * Vector4(x1, y1, z0, 1.0F),
                wm * Vector4(x0, y1, z0, 1.0F),
                wm * Vector4(x0, y0, z1, 1.0F),
                wm * Vector4(x1, y0, z1, 1.0F),
                wm * Vector4(x1, y1, z1, 1.0F),
                wm * Vector4(x0, y1, z1, 1.0F),
        };
        Vector3 v0 = Hp3(corners[0]);
        float minX = v0.x;
        float maxX = v0.x;
        float minY = v0.y;
        float maxY = v0.y;
        float minZ = v0.z;
        float maxZ = v0.z;
        for (int i = 1; i < 8; ++i) {
            const Vector3 v = Hp3(corners[static_cast<std::size_t>(i)]);
            minX = std::min(minX, v.x);
            maxX = std::max(maxX, v.x);
            minY = std::min(minY, v.y);
            maxY = std::max(maxY, v.y);
            minZ = std::min(minZ, v.z);
            maxZ = std::max(maxZ, v.z);
        }
        outWorld.box.minX = minX;
        outWorld.box.maxX = maxX;
        outWorld.box.minY = minY;
        outWorld.box.maxY = maxY;
        outWorld.box.minZ = minZ;
        outWorld.box.maxZ = maxZ;
        outWorld.bounds = outWorld.box;
        return;
    }

    if (outWorld.shape == TriggerVolume3DShape::Sphere) {
        outWorld.sphereCenter = Hp3(wm * Vector4(off.x, off.y, off.z, 1.0F));
        outWorld.sphereRadius = volume.GetRadius() * MaxMatrixScale(wm);
        BuildSphereBounds(outWorld.sphereCenter, outWorld.sphereRadius, outWorld.bounds);
        return;
    }

    const Vector3 localAxis = LocalCapsuleAxis(volume.GetCapsuleDirection());
    const float localRadius = volume.GetRadius();
    const float localHeight = volume.GetHeight();
    const float halfTotal = localHeight * 0.5F;
    float segmentHalf = halfTotal - localRadius;
    if (segmentHalf < 0.0F) {
        segmentHalf = 0.0F;
    }
    const Vector3 localA{
            off.x - localAxis.x * segmentHalf,
            off.y - localAxis.y * segmentHalf,
            off.z - localAxis.z * segmentHalf};
    const Vector3 localB{
            off.x + localAxis.x * segmentHalf,
            off.y + localAxis.y * segmentHalf,
            off.z + localAxis.z * segmentHalf};
    outWorld.capsule.pointA = Hp3(wm * Vector4(localA.x, localA.y, localA.z, 1.0F));
    outWorld.capsule.pointB = Hp3(wm * Vector4(localB.x, localB.y, localB.z, 1.0F));
    const float scale = MaxMatrixScale(wm);
    if (localHeight < localRadius * 2.0F) {
        outWorld.capsule.radius = halfTotal * scale;
        outWorld.capsule.pointA = Hp3(wm * Vector4(off.x, off.y, off.z, 1.0F));
        outWorld.capsule.pointB = outWorld.capsule.pointA;
    } else {
        outWorld.capsule.radius = localRadius * scale;
    }
    BuildCapsuleBounds(outWorld.capsule, outWorld.bounds);
}

bool TriggerVolume3DOverlapsProbe(const TriggerVolume3DWorld& volume, const DynamicCollider3DSim& probe) noexcept {
    if (!CollisionAabb3Overlaps(volume.bounds, probe.bounds)) {
        return false;
    }

    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    float pen = 0.0F;

    if (volume.shape == TriggerVolume3DShape::Box) {
        StaticCollider3DSim staticBox{};
        staticBox.shape = StaticCollider3DShape::Box;
        staticBox.aabb = volume.box;
        if (probe.shape == DynamicCollider3DShape::Sphere) {
            return StaticCollider3DOverlapsSphere(staticBox, probe.sphereCenter, probe.sphereRadius);
        }
        return ComputeCapsuleAabbContact(probe.capsule, volume.box, nx, ny, nz, pen);
    }

    if (volume.shape == TriggerVolume3DShape::Sphere) {
        if (probe.shape == DynamicCollider3DShape::Sphere) {
            return SphereSphereOverlap(volume.sphereCenter, volume.sphereRadius, probe.sphereCenter, probe.sphereRadius);
        }
        return ComputeSphereCapsuleContact(
                volume.sphereCenter, volume.sphereRadius, probe.capsule, nx, ny, nz, pen);
    }

    if (probe.shape == DynamicCollider3DShape::Sphere) {
        return ComputeSphereCapsuleContact(
                probe.sphereCenter, probe.sphereRadius, volume.capsule, nx, ny, nz, pen);
    }
    return ComputeCapsuleCapsuleContact(volume.capsule, probe.capsule, nx, ny, nz, pen);
}

void SimulateTriggerVolumes3D(
        GameWorld& world,
        const FrameTiming& timing,
        const TriggerVolume3DSettings& settings) {
    (void)timing;

    Array<TriggerVolumeEntry> volumes;
    Array<TriggerProbeEntry> probes;

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        if (auto* volume = object->GetComponent<TriggerVolume3DComponent>()) {
            if (object->GetComponent<TransformComponent>() == nullptr) {
                return;
            }
            TriggerVolumeEntry entry{};
            entry.owner = object;
            entry.volume = volume;
            BuildTriggerVolume3DWorld(*object, *volume, entry.world);
            volumes.PushBack(entry);
        }
    });

    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        DynamicCollider3DSim probe{};
        if (!TryBuildTriggerProbe3DFromObject(*object, settings, probe)) {
            return;
        }
        TriggerProbeEntry entry{};
        entry.owner = object;
        entry.sim = probe;
        probes.PushBack(entry);
    });

    for (std::size_t vi = 0; vi < volumes.GetSize(); ++vi) {
        TriggerVolumeEntry& volumeEntry = volumes[vi];
        TriggerVolume3DComponent& volume = *volumeEntry.volume;
        const Array<std::uint64_t> previous = volume.overlappingIds;
        Array<std::uint64_t> current;
        current.Clear();

        for (std::size_t pi = 0; pi < probes.GetSize(); ++pi) {
            const TriggerProbeEntry& probeEntry = probes[pi];
            if (probeEntry.owner == volumeEntry.owner) {
                continue;
            }
            if (!PassesTagFilter(volume, *probeEntry.owner)) {
                continue;
            }
            if (!TriggerVolume3DOverlapsProbe(volumeEntry.world, probeEntry.sim)) {
                continue;
            }
            const std::uint64_t probeId = probeEntry.owner->GetId();
            if (!ContainsId(current, probeId)) {
                current.PushBack(probeId);
            }
        }

        for (std::size_t i = 0; i < current.GetSize(); ++i) {
            const std::uint64_t id = current[i];
            if (ContainsId(previous, id)) {
                continue;
            }
            GameObject* other = world.FindGameObjectById(id);
            if (other != nullptr) {
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

}  // namespace Spark
