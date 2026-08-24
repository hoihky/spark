#pragma once

#include <cstdint>

namespace Spark {

/** Index range + glTF material index for one primitive within a shared vertex buffer. */
struct MeshSubmesh {
    std::uint32_t indexOffset = 0;
    std::uint32_t indexCount = 0;
    std::uint32_t materialIndex = 0;
};

/** SceneDrawItem::submeshIndex value meaning "draw the full mesh index buffer". */
constexpr std::uint32_t kSceneDrawFullSubmesh = UINT32_MAX;

}  // namespace Spark
