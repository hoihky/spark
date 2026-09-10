#pragma once

#include "spark/math/Vector2.hpp"

namespace Spark {

/** Single Gerstner wave (direction on XZ, amplitude in metres). */
class GerstnerWave {
public:
    [[nodiscard]] static GerstnerWave FromDirection(
            float directionX,
            float directionZ,
            float amplitude,
            float wavelength,
            float speed,
            float steepness) noexcept;

    Vector2 direction{1.0F, 0.0F};
    float amplitude = 0.0F;
    float wavelength = 1.0F;
    float speed = 1.0F;
    /** Steepness Q in [0, 1]; horizontal displacement scale. */
    float steepness = 0.0F;
};

}  // namespace Spark
