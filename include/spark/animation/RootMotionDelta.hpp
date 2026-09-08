#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

/** Per-frame root-motion translation extracted from a motion joint in skeleton space. */
struct RootMotionDelta {
    Vector3 translation{Vector3::Zero};
    bool valid = false;
};

}  // namespace Spark
