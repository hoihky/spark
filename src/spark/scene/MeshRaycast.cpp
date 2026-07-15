#include "spark/scene/MeshRaycast.hpp"

#include "spark/scene/Mesh.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] bool RayIntersectTriangle(
        const Vector3& ro,
        const Vector3& rd,
        const Vector3& v0,
        const Vector3& v1,
        const Vector3& v2,
        float& outT) noexcept {
    const Vector3 e1{v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    const Vector3 e2{v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    const Vector3 p{rd.y * e2.z - rd.z * e2.y, rd.z * e2.x - rd.x * e2.z, rd.x * e2.y - rd.y * e2.x};
    const float det = e1.x * p.x + e1.y * p.y + e1.z * p.z;
    if (std::fabs(det) < 1.0e-8F) {
        return false;
    }
    const float invDet = 1.0F / det;
    const Vector3 tvec{ro.x - v0.x, ro.y - v0.y, ro.z - v0.z};
    const float u = (tvec.x * p.x + tvec.y * p.y + tvec.z * p.z) * invDet;
    if (u < 0.0F || u > 1.0F) {
        return false;
    }
    const Vector3 q{tvec.y * e1.z - tvec.z * e1.y, tvec.z * e1.x - tvec.x * e1.z, tvec.x * e1.y - tvec.y * e1.x};
    const float v = (rd.x * q.x + rd.y * q.y + rd.z * q.z) * invDet;
    if (v < 0.0F || u + v > 1.0F) {
        return false;
    }
    const float t = (e2.x * q.x + e2.y * q.y + e2.z * q.z) * invDet;
    if (t <= 1.0e-6F) {
        return false;
    }
    outT = t;
    return true;
}

[[nodiscard]] Vector3 TransformPoint(const Matrix4& m, const Vector3& p) noexcept {
    const float x = m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12];
    const float y = m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13];
    const float z = m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14];
    const float w = m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15];
    if (std::fabs(w) < 1.0e-8F) {
        return {x, y, z};
    }
    const float iw = 1.0F / w;
    return {x * iw, y * iw, z * iw};
}

}  // namespace

bool TryRaycastMeshWorld(
        const Vector3& rayOriginWorld,
        const Vector3& rayDirWorld,
        const Mesh& mesh,
        const Matrix4& worldFromLocal,
        const float tMin,
        const float tMax,
        float& outT) noexcept {
    const Array<Mesh::Vertex>& verts = mesh.GetVertices();
    const Array<std::uint32_t>& indices = mesh.GetIndices();
    if (verts.IsEmpty() || indices.GetSize() < 3) {
        return false;
    }
    float bestT = tMax;
    bool hit = false;
    for (std::size_t ti = 0; ti + 2 < indices.GetSize(); ti += 3) {
        const std::uint32_t i0 = indices[ti];
        const std::uint32_t i1 = indices[ti + 1];
        const std::uint32_t i2 = indices[ti + 2];
        if (i0 >= verts.GetSize() || i1 >= verts.GetSize() || i2 >= verts.GetSize()) {
            continue;
        }
        const Vector3 w0 = TransformPoint(worldFromLocal, verts[i0].position);
        const Vector3 w1 = TransformPoint(worldFromLocal, verts[i1].position);
        const Vector3 w2 = TransformPoint(worldFromLocal, verts[i2].position);
        float t = 0.0F;
        if (!RayIntersectTriangle(rayOriginWorld, rayDirWorld, w0, w1, w2, t)) {
            continue;
        }
        if (t >= tMin && t < bestT) {
            bestT = t;
            hit = true;
        }
    }
    if (hit) {
        outT = bestT;
    }
    return hit;
}

bool TryRaycastSphereWorld(
        const Vector3& rayOriginWorld,
        const Vector3& rayDirWorld,
        const Vector3& centerWorld,
        const float radius,
        const float tMin,
        const float tMax,
        float& outT) noexcept {
    if (radius <= 0.0F) {
        return false;
    }
    const Vector3 oc{rayOriginWorld.x - centerWorld.x, rayOriginWorld.y - centerWorld.y,
            rayOriginWorld.z - centerWorld.z};
    const float b = oc.x * rayDirWorld.x + oc.y * rayDirWorld.y + oc.z * rayDirWorld.z;
    const float c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - radius * radius;
    const float disc = b * b - c;
    if (disc < 0.0F) {
        return false;
    }
    const float s = std::sqrt(disc);
    float t = -b - s;
    if (t < tMin) {
        t = -b + s;
    }
    if (t < tMin || t > tMax) {
        return false;
    }
    outT = t;
    return true;
}

}  // namespace Spark
