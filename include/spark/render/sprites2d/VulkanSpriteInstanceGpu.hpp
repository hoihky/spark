#pragma once

#include "spark/engine/SceneRenderParams.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/** std430 layout for <c>shaders/sprite_instance.glsl</c> (one per instanced quad). */
struct VulkanSpriteInstanceGpu {
    float model[16]{};
    float tint[4]{1.0F, 1.0F, 1.0F, 1.0F};
    float uvRect[4]{0.0F, 0.0F, 1.0F, 1.0F};
    float normalUvRect[4]{0.0F, 0.0F, 1.0F, 1.0F};
    std::int32_t textureLayer = -1;
    std::int32_t lightingMode = 0;
    std::int32_t normalTextureLayer = -1;
    std::int32_t rampTextureLayer = -1;
    float lightingA[4]{1.0F, 1.0F, 1.0F, 1.0F};
    float lightingB[4]{1.0F, 0.0F, 0.0F, 0.0F};
};

static_assert(sizeof(VulkanSpriteInstanceGpu) == 160U);

constexpr std::uint32_t kMaxQuadInstancesGpu =
        SceneRenderParams::MaxSprites + SceneRenderParams::MaxTilemapTiles;
constexpr std::size_t kQuadInstanceSsboBytes =
        sizeof(VulkanSpriteInstanceGpu) * static_cast<std::size_t>(kMaxQuadInstancesGpu);

/** @deprecated Use <c>kMaxQuadInstancesGpu</c>. */
constexpr std::uint32_t kMaxSpriteInstancesGpu = kMaxQuadInstancesGpu;
/** @deprecated Use <c>kQuadInstanceSsboBytes</c>. */
constexpr std::size_t kSpriteInstanceSsboBytes = kQuadInstanceSsboBytes;

}  // namespace Spark
