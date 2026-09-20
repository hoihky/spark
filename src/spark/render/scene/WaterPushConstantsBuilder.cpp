#include "spark/render/scene/WaterPushConstantsBuilder.hpp"

#include "spark/scene/water/GerstnerWave.hpp"
#include "spark/scene/water/WaterRenderingProfile.hpp"

#include <cstring>

namespace Spark {

WaterPushConstantsBuilder::WaterPushConstantsBuilder(const SceneWaterDraw& draw) noexcept {
    std::memcpy(push.model, draw.item.model.m, sizeof(push.model));
    push.baseColor[0] = draw.item.albedo.x;
    push.baseColor[1] = draw.item.albedo.y;
    push.baseColor[2] = draw.item.albedo.z;
    push.baseColor[3] = draw.item.opacity > 0.0F ? draw.item.opacity : 1.0F;
    push.timeSeconds = draw.waveTimeSeconds;
    push.waveCount = static_cast<std::int32_t>(draw.waveSettings.GetActiveWaveCount());
    push.shadowFlags = draw.item.shadowFlags;
    push.roughness = draw.item.roughness > 0.0F ? draw.item.roughness : 0.04F;
    push.waterLevelY = draw.waterLevelY;
    push.absorption = draw.absorption;
    push.deepColor[0] = draw.deepColor.x;
    push.deepColor[1] = draw.deepColor.y;
    push.deepColor[2] = draw.deepColor.z;
    push.deepColor[3] = 1.0F;
    push.foamStrength = draw.foamStrength;
    push.detailNormalStrength = draw.detailNormalStrength;
    push.shorelineFoamStrength = draw.shorelineFoamStrength;
    push.shorelineFoamMaxDepth = draw.shorelineFoamMaxDepth;
    push.tileAnchorXZ[0] = draw.waterTileAnchorXZ.x;
    push.tileAnchorXZ[1] = draw.waterTileAnchorXZ.y;
    const ResolvedWaterScreenSpaceReflection& ssr = draw.resolvedSsr;
    push.ssrEnabled = ssr.enabled ? 1.0F : 0.0F;
    push.ssrMaxRayDistance = ssr.maxRayDistance;
    push.ssrThickness = ssr.thickness;
    push.ssrStepScale = ssr.stepScale;
    push.ssrMaxSteps = ssr.maxSteps;
    push.ssrStrength = ssr.strength;
    push.ssrHorizonFadeStart = ssr.horizonFadeStart;
    push.ssrHorizonFadeEnd = ssr.horizonFadeEnd;
    push.ssrHalfRes = ssr.halfRes ? 1.0F : 0.0F;

    for (std::size_t wi = 0; wi < WaterWaveSettings::kMaxWaves; ++wi) {
        WriteWave(draw.waveSettings.GetWave(wi), push.waves[wi]);
    }
}

void WaterPushConstantsBuilder::WriteWave(const GerstnerWave& wave, WaterGerstnerWaveGpu& out) const noexcept {
    out.directionX = wave.direction.x;
    out.directionZ = wave.direction.y;
    out.amplitude = wave.amplitude;
    out.wavelength = wave.wavelength;
    out.speed = wave.speed;
    out.steepness = wave.steepness;
    out.padding[0] = 0.0F;
    out.padding[1] = 0.0F;
}

}  // namespace Spark
