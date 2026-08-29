#include "spark/editor/inspector/InspectorMath.hpp"

#include "spark/math/Constants.hpp"

#include <cmath>

namespace Spark::Editor {

namespace {

float NormalizeDegrees(float degrees) noexcept {
    float wrapped = std::fmod(degrees + 180.0F, 360.0F);
    if (wrapped < 0.0F) {
        wrapped += 360.0F;
    }
    return wrapped - 180.0F;
}

}  // namespace

Vector3 QuaternionToEulerDegrees(const Quaternion& rotation) noexcept {
    const Quaternion q = rotation.Normalized();
    const float sinrCosp = 2.0F * (q.w * q.x + q.y * q.z);
    const float cosrCosp = 1.0F - 2.0F * (q.x * q.x + q.y * q.y);
    const float pitch = std::atan2(sinrCosp, cosrCosp);

    const float sinp = 2.0F * (q.w * q.y - q.z * q.x);
    float yaw = 0.0F;
    if (std::fabs(sinp) >= 1.0F) {
        yaw = std::copysign(HalfPi, sinp);
    } else {
        yaw = std::asin(sinp);
    }

    const float sinyCosp = 2.0F * (q.w * q.z + q.x * q.y);
    const float cosyCosp = 1.0F - 2.0F * (q.y * q.y + q.z * q.z);
    const float roll = std::atan2(sinyCosp, cosyCosp);

    return {
            NormalizeDegrees(RadiansToDegrees(pitch)),
            NormalizeDegrees(RadiansToDegrees(yaw)),
            NormalizeDegrees(RadiansToDegrees(roll)),
    };
}

Quaternion EulerDegreesToQuaternion(const Vector3& eulerDegrees) noexcept {
    const float pitch = DegreesToRadians(eulerDegrees.x);
    const float yaw = DegreesToRadians(eulerDegrees.y);
    const float roll = DegreesToRadians(eulerDegrees.z);

    const float cy = std::cos(yaw * 0.5F);
    const float sy = std::sin(yaw * 0.5F);
    const float cp = std::cos(pitch * 0.5F);
    const float sp = std::sin(pitch * 0.5F);
    const float cr = std::cos(roll * 0.5F);
    const float sr = std::sin(roll * 0.5F);

    Quaternion q{
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy,
    };
    return q.Normalized();
}

}  // namespace Spark::Editor
