#pragma once

#include "spark/core/Utf8String.hpp"

namespace Spark {

/** Named marker on an animation clip at absolute time (seconds from clip start). */
struct AnimationClipEvent {
    float timeSeconds = 0.0F;
    Utf8String name;
};

}  // namespace Spark
