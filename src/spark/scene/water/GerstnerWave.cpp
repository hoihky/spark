#include "spark/scene/water/GerstnerWave.hpp"

#include <cmath>

namespace Spark {

GerstnerWave GerstnerWave::FromDirection(
        const float directionX,
        const float directionZ,
        const float amplitude,
        const float wavelength,
        const float speed,
        const float steepness) noexcept {
    GerstnerWave wave{};
    const float len = std::sqrt(directionX * directionX + directionZ * directionZ);
    if (len > 1.0e-6F) {
        wave.direction = {directionX / len, directionZ / len};
    } else {
        wave.direction = {1.0F, 0.0F};
    }
    wave.amplitude = amplitude;
    wave.wavelength = wavelength;
    wave.speed = speed;
    wave.steepness = steepness;
    return wave;
}

}  // namespace Spark
