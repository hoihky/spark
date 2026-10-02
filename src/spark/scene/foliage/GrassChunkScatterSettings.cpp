#include "spark/scene/foliage/GrassChunkScatterBuilder.hpp"

namespace Spark {

float GrassChunkScatterSettings::GetDistanceFadeStartMeters() const noexcept {
    const float end = maxViewDistanceMeters;
    const float band = end * distanceFadeOuterFraction;
    return end > band + 0.5F ? end - band : end * 0.55F;
}

}  // namespace Spark
