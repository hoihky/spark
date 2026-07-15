#pragma once

#include <cstdint>

namespace Spark {

/** How clip playback advances when time reaches the end of the clip duration. */
enum class AnimLoopMode : std::uint8_t {
    Loop = 0,
    Once = 1,
    Hold = 2,
};

}  // namespace Spark
