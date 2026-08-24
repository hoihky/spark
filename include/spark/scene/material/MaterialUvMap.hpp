#pragma once

#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

/** glTF textureInfo texcoord index + optional KHR_texture_transform. */
struct MaterialUvMap {
    std::uint32_t texCoordSet = 0;
    Vector2 uvScale{1.0F, 1.0F};
    Vector2 uvOffset{};
    float uvRotation = 0.0F;
};

}  // namespace Spark
