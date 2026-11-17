#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

struct CharacterLocomotionState {
    Vector3 velocity{Vector3::Zero};
    bool grounded = false;
};

}  // namespace Spark
