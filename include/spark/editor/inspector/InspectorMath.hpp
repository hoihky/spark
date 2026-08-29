#pragma once

#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark::Editor {

/** Euler angles in degrees (pitch X, yaw Y, roll Z) for Y-up scenes. */
[[nodiscard]] Vector3 QuaternionToEulerDegrees(const Quaternion& rotation) noexcept;
[[nodiscard]] Quaternion EulerDegreesToQuaternion(const Vector3& eulerDegrees) noexcept;

}  // namespace Spark::Editor
