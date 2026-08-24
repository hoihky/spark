#include "spark/math/AxisAlignedBox.hpp"
#include "spark/math/Frustum.hpp"

#include <algorithm>

namespace Spark {

void AxisAlignedBox::EncapsulateTransformedLocal(
        const Vector3& localMin, const Vector3& localMax, const Matrix4& world, Vector3& outMin, Vector3& outMax)
        noexcept {
    const Vector3 corners[8] = {
            {localMin.x, localMin.y, localMin.z},
            {localMax.x, localMin.y, localMin.z},
            {localMin.x, localMax.y, localMin.z},
            {localMax.x, localMax.y, localMin.z},
            {localMin.x, localMin.y, localMax.z},
            {localMax.x, localMin.y, localMax.z},
            {localMin.x, localMax.y, localMax.z},
            {localMax.x, localMax.y, localMax.z},
    };
    Vector3 w0 = world.TransformPoint(corners[0]);
    outMin = w0;
    outMax = w0;
    for (int i = 1; i < 8; ++i) {
        const Vector3 p = world.TransformPoint(corners[i]);
        outMin.x = std::min(outMin.x, p.x);
        outMin.y = std::min(outMin.y, p.y);
        outMin.z = std::min(outMin.z, p.z);
        outMax.x = std::max(outMax.x, p.x);
        outMax.y = std::max(outMax.y, p.y);
        outMax.z = std::max(outMax.z, p.z);
    }
}

Frustum Frustum::FromColumnMajorViewProjection(const Matrix4& viewProjection) noexcept {
    const float* m = viewProjection.m;
    Frustum f{};
    // clip = VP * p; visible: -w <= x <= w, -w <= y <= w, 0 <= z <= w
    const Vector4 left{m[0] + m[3], m[4] + m[7], m[8] + m[11], m[12] + m[15]};
    const Vector4 right{m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12]};
    const Vector4 bottom{m[1] + m[3], m[5] + m[7], m[9] + m[11], m[13] + m[15]};
    const Vector4 top{m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13]};
    const Vector4 nearP{m[2], m[6], m[10], m[14]};
    const Vector4 farP{m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]};
    f.planes[0] = left;
    f.planes[1] = right;
    f.planes[2] = bottom;
    f.planes[3] = top;
    f.planes[4] = nearP;
    f.planes[5] = farP;
    return f;
}

bool Frustum::IntersectsAxisAlignedBox(const Vector3& boxMin, const Vector3& boxMax) const noexcept {
    for (int i = 0; i < 6; ++i) {
        const Vector4& pl = planes[i];
        const Vector3 p{pl.x > 0.0F ? boxMax.x : boxMin.x, pl.y > 0.0F ? boxMax.y : boxMin.y,
                pl.z > 0.0F ? boxMax.z : boxMin.z};
        const float d = pl.x * p.x + pl.y * p.y + pl.z * p.z + pl.w;
        if (d < 0.0F) {
            return false;
        }
    }
    return true;
}

bool Frustum::Intersects(const AxisAlignedBox& box) const noexcept {
    return IntersectsAxisAlignedBox(box.min, box.max);
}

}  // namespace Spark
