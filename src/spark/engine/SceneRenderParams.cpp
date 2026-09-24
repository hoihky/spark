#include "spark/engine/SceneRenderParams.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

float ClampFloat(const float value, const float lo, const float hi) noexcept {
    return std::max(lo, std::min(value, hi));
}

}  // namespace

void SceneRenderParams::Sanitize() noexcept {
    lightIntensity = std::max(lightIntensity, 0.0F);
    if (lightDirectionWorld.LengthSquared() > 1.0e-8F) {
        lightDirectionWorld = lightDirectionWorld.Normalized();
    }

    shadowBias = ClampFloat(shadowBias, 0.0F, 0.05F);
    shadowNormalBias = ClampFloat(shadowNormalBias, 0.0F, 0.25F);

    if (exposure < 0.0F) {
        exposure = 0.0F;
    }
    if (shadowCascadeNear < 0.0F) {
        shadowCascadeNear = 0.0F;
    }
    if (shadowCascadeFar < 0.0F) {
        shadowCascadeFar = 0.0F;
    }
    if (shadowDistanceMax < 0.0F) {
        shadowDistanceMax = 0.0F;
    }
    if (shadowFadeStartRatio > 0.0F) {
        shadowFadeStartRatio = ClampFloat(shadowFadeStartRatio, 0.05F, 0.99F);
    }
    if (ambientScale < 0.0F) {
        ambientScale = 0.0F;
    }

    timeOfDay = std::fmod(timeOfDay, 1.0F);
    if (timeOfDay < 0.0F) {
        timeOfDay += 1.0F;
    }

    ssaoRadius = ClampFloat(ssaoRadius, 0.01F, 4.0F);
    ssaoBias = ClampFloat(ssaoBias, 0.0F, 0.25F);
    ssaoStrength = ClampFloat(ssaoStrength, 0.0F, 2.0F);

    fogDensity = std::max(fogDensity, 0.0F);
    fogStart = std::max(fogStart, 0.0F);
    fogEnd = std::max(fogEnd, fogStart + 0.01F);

    iblIntensity = std::max(iblIntensity, 0.0F);

    if (worldViewportScissorEnabled) {
        worldViewportScissorW = std::max(worldViewportScissorW, 0.0F);
        worldViewportScissorH = std::max(worldViewportScissorH, 0.0F);
    }
}

}  // namespace Spark
