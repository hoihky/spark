#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

/** Per-frame locomotion and combat requests (built from input or AI). */
struct CharacterIntent {
    /** Desired horizontal world velocity (m/s); Y ignored by kinematic motor. */
    Vector3 wishVelocityWorld{Vector3::Zero};
    bool sprint = false;
    bool requestAttack = false;
    bool requestDodge = false;
};

}  // namespace Spark
