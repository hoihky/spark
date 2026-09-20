#pragma once

#include <cstdint>

namespace Spark {

/** Color format for offscreen <c>RenderTexture</c> targets (GPU encoding is backend-specific). */
enum class RenderTextureFormat : std::uint8_t {
    /** Scene HDR path: R16G16B16A16 SFLOAT (matches <c>VulkanHdrTonemapPass</c>). */
    HdrRGBA16Float = 0,
    Rgba8Unorm,
};

}  // namespace Spark
