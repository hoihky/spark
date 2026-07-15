#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * TRS transform. TransformPosition applies scale, then rotation, then translation:
 *   world = rotation * (scale * local) + translation
 */
struct Transform {
    Vector3 translation{Vector3::Zero};
    Quaternion rotation{Quaternion::Identity};
    Vector3 scale{Vector3::One};

    static const Transform Identity;

    [[nodiscard]] static Transform FromTranslation(const Vector3& t) noexcept {
        Transform r;
        r.translation = t;
        return r;
    }

    [[nodiscard]] static Transform FromRotation(const Quaternion& q) noexcept {
        Transform r;
        r.rotation = q;
        return r;
    }

    [[nodiscard]] static Transform FromScale(const Vector3& s) noexcept {
        Transform r;
        r.scale = s;
        return r;
    }

    [[nodiscard]] Matrix4 ToMatrix4() const noexcept {
        const Matrix4 s = Matrix4::Scale(scale);
        const Matrix4 r = Matrix4::Rotation(rotation);
        const Matrix4 t = Matrix4::Translation(translation);
        return t * r * s;
    }

    [[nodiscard]] Vector3 TransformPosition(const Vector3& local) const noexcept {
        const Vector3 scaled = {local.x * scale.x, local.y * scale.y, local.z * scale.z};
        return rotation.RotateVector(scaled) + translation;
    }

    /** Undo TransformPosition (ignores translation component on vectors). */
    [[nodiscard]] Vector3 InverseTransformPosition(const Vector3& world) const noexcept {
        const Quaternion rInv = rotation.Conjugate().Normalized();
        Vector3 v = world - translation;
        v = rInv.RotateVector(v);
        return {v.x / scale.x, v.y / scale.y, v.z / scale.z};
    }

    /** Applies scale then rotation (no translation). */
    [[nodiscard]] Vector3 TransformVector(const Vector3& local) const noexcept {
        const Vector3 scaled = {local.x * scale.x, local.y * scale.y, local.z * scale.z};
        return rotation.RotateVector(scaled);
    }

    [[nodiscard]] Vector3 InverseTransformVector(const Vector3& world) const noexcept {
        const Quaternion rInv = rotation.Conjugate().Normalized();
        const Vector3 v = rInv.RotateVector(world);
        return {v.x / scale.x, v.y / scale.y, v.z / scale.z};
    }
};

inline constexpr Transform Transform::Identity{Vector3::Zero, Quaternion::Identity, Vector3::One};

}  // namespace Spark
