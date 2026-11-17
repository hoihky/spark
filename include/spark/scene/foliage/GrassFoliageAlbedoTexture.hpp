#pragma once

#include "spark/core/SharedPtr.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

/** Procedural vertical grass blade albedo + alpha (for foliage alpha test). */
class GrassFoliageAlbedoTexture {
public:
    [[nodiscard]] static SharedPtr<Texture2D> CreateSharedBladeAlbedo();
};

}  // namespace Spark
