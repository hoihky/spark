#include "spark/demo/ShellDemoSceneUtil.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

int DrawSortLayer(SceneMeshSlot s) {
    switch (s) {
    case SceneMeshSlot::GroundPlane:
        return 0;
    case SceneMeshSlot::UnitCube:
        return 1;
    case SceneMeshSlot::Custom:
        return 2;
    }
    return 1;
}

}  // namespace

/** Orthonormal columns col0,col1,col2 (rotation matrix) → unit quaternion (x,y,z,w). */
Quaternion QuaternionFromRotationColumns(
        const Vector3& col0, const Vector3& col1, const Vector3& col2) noexcept {
    const float m00 = col0.x;
    const float m01 = col1.x;
    const float m02 = col2.x;
    const float m10 = col0.y;
    const float m11 = col1.y;
    const float m12 = col2.y;
    const float m20 = col0.z;
    const float m21 = col1.z;
    const float m22 = col2.z;
    const float tr = m00 + m11 + m22;
    if (tr > 0.0F) {
        const float s = 0.5F / std::sqrt(tr + 1.0F);
        return {(m21 - m12) * s, (m02 - m20) * s, (m10 - m01) * s, 0.25F / s};
    }
    if (m00 > m11 && m00 > m22) {
        const float s = 2.0F * std::sqrt(1.0F + m00 - m11 - m22);
        return {0.25F * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
    }
    if (m11 > m22) {
        const float s = 2.0F * std::sqrt(1.0F + m11 - m00 - m22);
        return {(m01 + m10) / s, 0.25F * s, (m12 + m21) / s, (m02 - m20) / s};
    }
    const float s = 2.0F * std::sqrt(1.0F + m22 - m00 - m11);
    return {(m02 + m20) / s, (m12 + m21) / s, 0.25F * s, (m10 - m01) / s};
}

int DrawSortKey(const SceneDrawItem& it) {
    if (it.skyMode != SceneSkyMode::None) {
        return -100;
    }
    return DrawSortLayer(it.mesh);
}

void StableSortDrawItems(Array<SceneDrawItem>& items) {
    const std::size_t n = items.GetSize();
    for (std::size_t i = 1; i < n; ++i) {
        SceneDrawItem key = items[i];
        std::size_t j = i;
        while (j > 0 && DrawSortKey(items[j - 1]) > DrawSortKey(key)) {
            items[j] = items[j - 1];
            --j;
        }
        items[j] = key;
    }
}

[[nodiscard]] bool TerrainScreenToWorldRay(
        int fbW,
        int fbH,
        float px,
        float py,
        const Matrix4& invViewProj,
        Vector3& outOrigin,
        Vector3& outDir) {
    if (fbW <= 0 || fbH <= 0) {
        return false;
    }
    // Match shaders/scene.frag sky path: ndc = vec2(sx*2-1, sy*2-1) with sy = (fragY+0.5)/vp.h (Y down).
    // Cursor pixels use the same top-left origin as gl_FragCoord, so do not use OpenGL-style ndcY flip here.
    const float ndcX = (2.0F * px / static_cast<float>(fbW)) - 1.0F;
    const float ndcY = (2.0F * py / static_cast<float>(fbH)) - 1.0F;
    const Vector4 p0 = invViewProj * Vector4(ndcX, ndcY, 0.0F, 1.0F);
    const Vector4 p1 = invViewProj * Vector4(ndcX, ndcY, 1.0F, 1.0F);
    if (std::fabs(p0.w) < 1.0e-6F || std::fabs(p1.w) < 1.0e-6F) {
        return false;
    }
    const Vector3 o = (p0 * (1.0F / p0.w)).ToVector3();
    Vector3 d = (p1 * (1.0F / p1.w)).ToVector3() - o;
    if (d.LengthSquared() < 1.0e-12F) {
        return false;
    }
    d = d.Normalized();
    outOrigin = o;
    outDir = d;
    return true;
}

}  // namespace Spark
