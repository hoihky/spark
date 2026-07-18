#include "spark/render/shadow/VulkanDirectionalShadowCascadeMath.hpp"

#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

/*
 * Shadow pipeline trace (single directional cascade):
 * 1) VulkanSceneUniformWriter: lightDir = normalize(scene.lightDirectionWorld) — toward sun.
 * 2) ComputeWorldToShadowClip(...) per cascade → worldToShadowClip[i] = ZFlip * lightProj * lightView.
 * 3) VulkanDirectionalShadowPass::Record: same matrix in ShadowPushConstants.lightViewProj.
 * 4) scene.frag: light clip from vWorldPos; depth compare uses slope-scaled bias.
 * 5) scene.vert: transpose(inverse(mat3(model))) * n for non-uniform scale.
 */
const Matrix4& VulkanDirectionalShadowCascadeMath::ShadowClipLinearZFlip() noexcept {
    static const Matrix4 k = [] {
        Matrix4 r = Matrix4::IdentityMatrix();
        r.m[10] = -1.0F;
        r.m[14] = 1.0F;
        return r;
    }();
    return k;
}

float VulkanDirectionalShadowCascadeMath::ComputeSplitDistance(
        const float nearDist,
        const float farDist,
        const int splitIndex,
        const int splitCount,
        const float lambda) noexcept {
    const float p = static_cast<float>(splitIndex) / static_cast<float>(splitCount);
    const float logSplit = nearDist * std::pow(farDist / std::max(nearDist, 1e-3F), p);
    const float uniSplit = nearDist + (farDist - nearDist) * p;
    return lambda * logSplit + (1.0F - lambda) * uniSplit;
}

bool VulkanDirectionalShadowCascadeMath::ComputeWorldToShadowClip(
        const Matrix4& viewProj,
        const Vector3& lightDirectionWorldTowardLight,
        const float distNear,
        const float distFar,
        const std::uint32_t cascadeTileSize,
        Matrix4& outWorldToShadowClip) {
    Matrix4 invVp{};
    if (!viewProj.TryInvert(invVp)) {
        return false;
    }
    const Vector4 ndcCorners[8] = {
            {-1.0F, -1.0F, 0.0F, 1.0F},
            {1.0F, -1.0F, 0.0F, 1.0F},
            {1.0F, 1.0F, 0.0F, 1.0F},
            {-1.0F, 1.0F, 0.0F, 1.0F},
            {-1.0F, -1.0F, 1.0F, 1.0F},
            {1.0F, -1.0F, 1.0F, 1.0F},
            {1.0F, 1.0F, 1.0F, 1.0F},
            {-1.0F, 1.0F, 1.0F, 1.0F},
    };
    Vector3 center{};
    for (const Vector4& c : ndcCorners) {
        const Vector4 w = invVp * c;
        const float iw = (w.w != 0.0F) ? (1.0F / w.w) : 1.0F;
        center.x += w.x * iw;
        center.y += w.y * iw;
        center.z += w.z * iw;
    }
    constexpr float inv8 = 1.0F / 8.0F;
    center.x *= inv8;
    center.y *= inv8;
    center.z *= inv8;

    const Vector3 L = lightDirectionWorldTowardLight.Normalized();
    constexpr float kPullDist = 260.0F;
    const Vector3 lightPos = center + L * kPullDist;

    Vector3 up = Vector3::UnitY;
    if (std::fabs(Vector3::Dot(L, up)) > 0.95F) {
        up = Vector3::UnitX;
    }
    const Matrix4 lightView = Matrix4::LookAt(lightPos, center, up);

    float minX = 1e30F;
    float maxX = -1e30F;
    float minY = 1e30F;
    float maxY = -1e30F;
    float minZ = 1e30F;
    float maxZ = -1e30F;
    float wxMin = 1e30F;
    float wxMax = -1e30F;
    float wzMin = 1e30F;
    float wzMax = -1e30F;

    auto accumulateWorldInLight = [&](const Vector3& pWorld) {
        wxMin = std::min(wxMin, pWorld.x);
        wxMax = std::max(wxMax, pWorld.x);
        wzMin = std::min(wzMin, pWorld.z);
        wzMax = std::max(wzMax, pWorld.z);
        const Vector4 lv = lightView * Vector4(pWorld, 1.0F);
        minX = std::min(minX, lv.x);
        maxX = std::max(maxX, lv.x);
        minY = std::min(minY, lv.y);
        maxY = std::max(maxY, lv.y);
        minZ = std::min(minZ, lv.z);
        maxZ = std::max(maxZ, lv.z);
    };

    auto accumulateFromNdc = [&](const Vector4& ndcHom) {
        const Vector4 worldH = invVp * ndcHom;
        const float iw = (worldH.w != 0.0F) ? (1.0F / worldH.w) : 1.0F;
        const Vector3 p{worldH.x * iw, worldH.y * iw, worldH.z * iw};
        accumulateWorldInLight(p);
    };

    constexpr float kFrustumNear = 0.12F;
    constexpr float kFrustumFar = 400.0F;
    const float frustumSpan = std::max(kFrustumFar - kFrustumNear, 1e-3F);
    const float tNear = std::clamp((distNear - kFrustumNear) / frustumSpan, 0.0F, 1.0F);
    const float tFar = std::clamp((distFar - kFrustumNear) / frustumSpan, 0.0F, 1.0F);

    auto accumulateFrustumSlice = [&](const float t0, const float t1) {
        for (const Vector4& c : ndcCorners) {
            const Vector4 worldH0 = invVp * Vector4(c.x, c.y, 0.0F, 1.0F);
            const Vector4 worldH1 = invVp * Vector4(c.x, c.y, 1.0F, 1.0F);
            const float iw0 = (worldH0.w != 0.0F) ? (1.0F / worldH0.w) : 1.0F;
            const float iw1 = (worldH1.w != 0.0F) ? (1.0F / worldH1.w) : 1.0F;
            const Vector3 p0{worldH0.x * iw0, worldH0.y * iw0, worldH0.z * iw0};
            const Vector3 p1{worldH1.x * iw1, worldH1.y * iw1, worldH1.z * iw1};
            accumulateWorldInLight(p0 + (p1 - p0) * t0);
            accumulateWorldInLight(p0 + (p1 - p0) * t1);
        }
    };
    accumulateFrustumSlice(tNear, tFar);

    const Vector4 extraNdcXY[] = {
            {0.0F, 0.0F, 0.0F, 1.0F},
            {0.0F, -1.0F, 0.0F, 1.0F},
            {0.0F, 1.0F, 0.0F, 1.0F},
            {-1.0F, 0.0F, 0.0F, 1.0F},
            {1.0F, 0.0F, 0.0F, 1.0F},
    };
    for (const float tz : {tNear, tFar}) {
        for (const Vector4& c : extraNdcXY) {
            accumulateFromNdc(Vector4(c.x, c.y, tz, 1.0F));
        }
    }

    const float gh = kSceneGroundHalfExtent;
    const Vector3 groundCorners[4] = {
            {-gh, 0.0F, -gh},
            {-gh, 0.0F, gh},
            {gh, 0.0F, -gh},
            {gh, 0.0F, gh},
    };
    for (const Vector3& gp : groundCorners) {
        accumulateWorldInLight(gp);
    }

    if (wxMax > wxMin + 1e-3F && wzMax > wzMin + 1e-3F) {
        constexpr int kGridN = 8;
        for (int ix = 0; ix <= kGridN; ++ix) {
            const float tx = static_cast<float>(ix) / static_cast<float>(kGridN);
            const float wx = wxMin * (1.0F - tx) + wxMax * tx;
            for (int iz = 0; iz <= kGridN; ++iz) {
                const float tz = static_cast<float>(iz) / static_cast<float>(kGridN);
                const float wz = wzMin * (1.0F - tz) + wzMax * tz;
                accumulateWorldInLight({wx, 0.0F, wz});
            }
        }
    }

    const float spanX = maxX - minX;
    const float spanY = maxY - minY;
    const float spanZ = maxZ - minZ;
    const float padX = std::max(18.0F, spanX * 0.26F);
    const float padY = std::max(18.0F, spanY * 0.26F);
    minX -= padX;
    maxX += padX;
    minY -= padY;
    maxY += padY;
    const float padZ = std::max(48.0F, spanZ * 0.18F);
    minZ -= padZ;
    maxZ += padZ;

    const float spanSnapX = maxX - minX;
    const float spanSnapY = maxY - minY;
    const float texelX = spanSnapX / static_cast<float>(cascadeTileSize);
    const float texelY = spanSnapY / static_cast<float>(cascadeTileSize);
    if (texelX > 1e-6F && texelY > 1e-6F) {
        const float cx = 0.5F * (minX + maxX);
        const float cy = 0.5F * (minY + maxY);
        minX = std::floor((minX - cx) / texelX) * texelX + cx;
        maxX = minX + spanSnapX;
        minY = std::floor((minY - cy) / texelY) * texelY + cy;
        maxY = minY + spanSnapY;
    }

    float zSpan = maxZ - minZ;
    constexpr float kMinLightSpaceZSpan = 2.0F;
    if (zSpan < kMinLightSpaceZSpan) {
        const float zMid = 0.5F * (minZ + maxZ);
        minZ = zMid - 0.5F * kMinLightSpaceZSpan;
        maxZ = zMid + 0.5F * kMinLightSpaceZSpan;
    }

    const Matrix4 lightProj = Matrix4::OrthographicVulkan(minX, maxX, minY, maxY, minZ, maxZ);
    outWorldToShadowClip = ShadowClipLinearZFlip() * lightProj * lightView;
    return true;
}

}  // namespace Spark
