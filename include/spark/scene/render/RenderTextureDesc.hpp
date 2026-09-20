#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/render/RenderTextureFormat.hpp"

#include <cstdint>

namespace Spark {

/** Creation parameters for an offscreen color (+ optional depth) render target. */
struct RenderTextureDesc {
    std::uint32_t width = 1;
    std::uint32_t height = 1;
    RenderTextureFormat colorFormat = RenderTextureFormat::HdrRGBA16Float;
    bool includeDepth = true;
    Utf8String name{};
};

}  // namespace Spark
