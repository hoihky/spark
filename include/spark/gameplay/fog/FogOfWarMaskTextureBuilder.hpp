#pragma once

#include "spark/gameplay/fog/FogOfWarExplorationMap.hpp"

namespace Spark {

class Texture2D;

/** Builds RGBA fog mask textures from <c>FogOfWarExplorationMap</c> (Adapter to GPU upload). */
class FogOfWarMaskTextureBuilder final {
public:
    void RebuildMaskTexture(const FogOfWarExplorationMap& exploration, Texture2D& texture) const noexcept;
};

}  // namespace Spark
