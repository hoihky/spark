#pragma once

#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

namespace Spark {

/** Per-blade albedo variation from deterministic hashes (F2 scatter). */
class GrassInstanceTintGenerator {
public:
    [[nodiscard]] static Vector4 BuildTint(const Vector3& baseTint, float hashA, float hashB, float hashC) noexcept;
};

}  // namespace Spark
