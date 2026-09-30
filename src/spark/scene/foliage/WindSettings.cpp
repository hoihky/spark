#include "spark/scene/foliage/WindSettings.hpp"

#include <cmath>

namespace Spark {

Vector3 WindSettings::NormalizedDirectionXZ() const noexcept {
    Vector3 xz{direction.x, 0.0F, direction.z};
    const float len = std::sqrt(xz.x * xz.x + xz.z * xz.z);
    if (len < 1.0e-4F) {
        return {1.0F, 0.0F, 0.0F};
    }
    xz.x /= len;
    xz.z /= len;
    return xz;
}

}  // namespace Spark
