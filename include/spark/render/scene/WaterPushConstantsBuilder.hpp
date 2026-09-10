#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/scene/VulkanWaterPushConstants.hpp"

namespace Spark {

/** Builds GPU push constants for a single water draw. */
class WaterPushConstantsBuilder {
public:
    explicit WaterPushConstantsBuilder(const SceneWaterDraw& draw) noexcept;

    [[nodiscard]] const WaterPushConstants& Get() const noexcept { return push; }

private:
    void WriteWave(const GerstnerWave& wave, WaterGerstnerWaveGpu& out) const noexcept;

    WaterPushConstants push{};
};

}  // namespace Spark
