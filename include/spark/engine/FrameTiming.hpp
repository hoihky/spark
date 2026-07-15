#pragma once

#include <cstdint>

namespace Spark {

struct FrameTiming {
    float deltaTimeSeconds = 0.0F;
    float totalTimeSeconds = 0.0F;
    std::uint64_t frameIndex = 0;
};

}  // namespace Spark
