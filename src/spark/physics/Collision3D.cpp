#include "spark/physics/Collision3D.hpp"

#include "spark/ecs/components/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/SphereCollider3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 Hp3(const Vector4& p) noexcept {
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

}  // namespace

bool CollisionAabb3Overlaps(const CollisionAabb3& a, const CollisionAabb3& b) noexcept {
    return a.minX < b.maxX && a.maxX > b.minX && a.minY < b.maxY && a.maxY > b.minY && a.minZ < b.maxZ &&
            a.maxZ > b.minZ;
}

bool CollisionAabb3OverlapsSphere(const CollisionAabb3& a, const Vector3& center, const float radius) noexcept {
    const float qx = std::clamp(center.x, a.minX, a.maxX);
    const float qy = std::clamp(center.y, a.minY, a.maxY);
    const float qz = std::clamp(center.z, a.minZ, a.maxZ);
    const float dx = center.x - qx;
    const float dy = center.y - qy;
    const float dz = center.z - qz;
    const float rr = radius * radius;
    return dx * dx + dy * dy + dz * dz <= rr + 1.0e-8F;
}

bool CollisionAabb3OverlapsSphereInflated(
        const CollisionAabb3& a,
        const Vector3& center,
        const float baseRadius,
        const float inflateRadius) noexcept {
    const float r = baseRadius + std::max(0.0F, inflateRadius);
    const float qx = std::clamp(center.x, a.minX, a.maxX);
    const float qy = std::clamp(center.y, a.minY, a.maxY);
    const float qz = std::clamp(center.z, a.minZ, a.maxZ);
    const float dx = center.x - qx;
    const float dy = center.y - qy;
    const float dz = center.z - qz;
    const float rr = r * r;
    return dx * dx + dy * dy + dz * dz <= rr + 1.0e-8F;
}

void ComputeBoxCollider3WorldAabb(
        GameObject& owner,
        const BoxCollider3DComponent& collider,
        CollisionAabb3& outWorld) noexcept {
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector3 off = collider.GetOffset();
    const Vector3 he = collider.GetHalfExtents();
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
    outWorld.minX = minX;
    outWorld.maxX = maxX;
    outWorld.minY = minY;
    outWorld.maxY = maxY;
    outWorld.minZ = minZ;
    outWorld.maxZ = maxZ;
}

void ComputeSphereCollider3World(
        GameObject& owner,
        const SphereCollider3DComponent& collider,
        Vector3& outCenter,
        float& outRadius) noexcept {
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector3 off = collider.GetOffset();
    const Vector4 pc = wm * Vector4(off.x, off.y, off.z, 1.0F);
    outCenter = Hp3(pc);
    const float sx = std::sqrt(wm.m[0] * wm.m[0] + wm.m[1] * wm.m[1] + wm.m[2] * wm.m[2]);
    const float sy = std::sqrt(wm.m[4] * wm.m[4] + wm.m[5] * wm.m[5] + wm.m[6] * wm.m[6]);
    const float sz = std::sqrt(wm.m[8] * wm.m[8] + wm.m[9] * wm.m[9] + wm.m[10] * wm.m[10]);
    const float scale = std::max({sx, sy, sz});
    outRadius = collider.GetRadius() * scale;
}

bool ContributesStaticCollider3D(GameObject& object) noexcept {
    const BoxCollider3DComponent* box = object.GetComponent<BoxCollider3DComponent>();
    if (box == nullptr) {
        return false;
    }
    const Rigidbody3DComponent* rb = object.GetComponent<Rigidbody3DComponent>();
    if (rb == nullptr) {
        return true;
    }
    return rb->GetBodyType() != RigidbodyBodyType3D::Dynamic;
}

}  // namespace Spark
