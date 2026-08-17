#include "spark/scene/TextureMipLevel.hpp"

#include "spark/core/Utility.hpp"

namespace Spark {

TextureMipLevel::TextureMipLevel(
        const std::uint32_t levelWidth,
        const std::uint32_t levelHeight,
        Array<std::uint8_t> levelBytes)
        : width(levelWidth), height(levelHeight), bytes(MoveTemp(levelBytes)) {}

}  // namespace Spark
