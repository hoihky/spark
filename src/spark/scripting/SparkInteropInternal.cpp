#include "spark/scripting/SparkInteropInternal.hpp"

#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

namespace Spark::Scripting {

ComponentKind ToComponentKind(const SparkComponentKind kind) noexcept {
    return static_cast<ComponentKind>(static_cast<std::uint32_t>(kind));
}

Vector3 ToVector3(const SparkVector3 v) noexcept {
    return {v.x, v.y, v.z};
}

SparkVector3 FromVector3(const Vector3& v) noexcept {
    return {v.x, v.y, v.z};
}

Vector4 ToVector4(const SparkVector4 v) noexcept {
    return {v.x, v.y, v.z, v.w};
}

SparkVector4 FromVector4(const Vector4& v) noexcept {
    return {v.x, v.y, v.z, v.w};
}

Vector2 ToVector2(const SparkVector2 v) noexcept {
    return {v.x, v.y};
}

SparkVector2 FromVector2(const Vector2& v) noexcept {
    return {v.x, v.y};
}

Quaternion ToQuaternion(const SparkQuaternion q) noexcept {
    return {q.x, q.y, q.z, q.w};
}

SparkQuaternion FromQuaternion(const Quaternion& q) noexcept {
    return {q.x, q.y, q.z, q.w};
}

Matrix4 ToMatrix4(const SparkMatrix4& m) noexcept {
    Matrix4 out{};
    for (int i = 0; i < 16; ++i) {
        out.m[i] = m.m[i];
    }
    return out;
}

SparkMatrix4 FromMatrix4(const Matrix4& m) noexcept {
    SparkMatrix4 out{};
    for (int i = 0; i < 16; ++i) {
        out.m[i] = m.m[i];
    }
    return out;
}

}  // namespace Spark::Scripting
