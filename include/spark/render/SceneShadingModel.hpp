#pragma once

#include <cstdint>

namespace Spark {

/**
 * Per-draw lit mesh shading path (see <c>shaders/scene.frag</c> + <c>VulkanRenderer::ModelPushConstants</c>).
 * <c>LitPbr</c> is Cook–Torrance; <c>ToonCel</c> uses banded diffuse, stepped specular, and rim from material fields.
 */
enum class SceneShadingModel : std::uint8_t {
    LitPbr = 0,
    ToonCel = 1,
};

}  // namespace Spark
