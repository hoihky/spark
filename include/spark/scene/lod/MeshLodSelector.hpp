#pragma once

#include "spark/core/Array.hpp"
#include "spark/scene/lod/MeshLodLevel.hpp"

#include <cstdint>

namespace Spark {

/** Distance + hysteresis LOD index selection (OPEN_WORLD C1–C2, FOLIAGE F4). */
class MeshLodSelector final {
public:
    [[nodiscard]] static std::uint32_t SelectIndex(
            const Array<MeshLodLevel>& levels,
            float cameraDistanceMeters,
            std::uint32_t& activeIndexInOut,
            float hysteresisMeters) noexcept;
};

}  // namespace Spark