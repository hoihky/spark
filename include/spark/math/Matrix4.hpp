#pragma once

#include "spark/math/Constants.hpp"
#include "spark/math/Matrix3.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

#include <cmath>

namespace Spark {

/**
 * 4x4 matrix, column-major (matches typical GLSL mat4 layout).
 * Index: column c (0..3), row r (0..3) -> m[c * 4 + r].
 */
struct Matrix4 {
    float m[16]{};  // column-major

    static const Matrix4 Identity;

    static Matrix4 IdentityMatrix() noexcept {
        Matrix4 r{};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0F;
        return r;
    }

    static Matrix4 Translation(const Vector3& t) noexcept {
        Matrix4 r = IdentityMatrix();
        r.m[12] = t.x;
        r.m[13] = t.y;
        r.m[14] = t.z;
        return r;
    }

    static Matrix4 Scale(const Vector3& s) noexcept {
        Matrix4 r{};
        r.m[0] = s.x;
        r.m[5] = s.y;
        r.m[10] = s.z;
        r.m[15] = 1.0F;
        return r;
    }

    static Matrix4 Scale(float uniform) noexcept { return Scale({uniform, uniform, uniform}); }

    /** Rotation only (homogeneous row 4 = 0,0,0,1). */
    static Matrix4 Rotation(const Quaternion& q) noexcept {
        const Quaternion n = q.Normalized();
        const float x = n.x;
        const float y = n.y;
        const float z = n.z;
        const float w = n.w;
        const float xx = x * x;
        const float yy = y * y;
        const float zz = z * z;
        const float xy = x * y;
        const float xz = x * z;
        const float yz = y * z;
        const float wx = w * x;
        const float wy = w * y;
        const float wz = w * z;
        Matrix4 r = IdentityMatrix();
        r.m[0] = 1.0F - 2.0F * (yy + zz);
        r.m[1] = 2.0F * (xy + wz);
        r.m[2] = 2.0F * (xz - wy);
        r.m[4] = 2.0F * (xy - wz);
        r.m[5] = 1.0F - 2.0F * (xx + zz);
        r.m[6] = 2.0F * (yz + wx);
        r.m[8] = 2.0F * (xz + wy);
        r.m[9] = 2.0F * (yz - wx);
        r.m[10] = 1.0F - 2.0F * (xx + yy);
        return r;
    }

    /**
     * Vertical field of view in radians, aspect = width/height.
     * Depth range [nearZ, farZ] maps to [0, 1] (Vulkan / D3D style clip Z).
     */
    static Matrix4 Perspective(float verticalFovYRad, float aspect, float nearZ, float farZ) noexcept {
        const float tanHalf = std::tan(verticalFovYRad * 0.5F);
        const float f = 1.0F / tanHalf;
        Matrix4 r{};
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = farZ / (nearZ - farZ);
        r.m[11] = -1.0F;
        r.m[14] = (nearZ * farZ) / (nearZ - farZ);
        return r;
    }

    /**
     * Same as Perspective() but flips clip-space Y for Vulkan with a normal (positive-height) viewport.
     * Prefer this over a negative viewport height (better MoltenVK / portability).
     */
    static Matrix4 PerspectiveVulkan(float verticalFovYRad, float aspect, float nearZ, float farZ) noexcept {
        Matrix4 r = Perspective(verticalFovYRad, aspect, nearZ, farZ);
        r.m[5] *= -1.0F;
        return r;
    }

    /**
     * Orthographic projection: maps axis-aligned box [left,right]×[bottom,top]×[nearZ,farZ] to NDC.
     * X → [-1,1], Y → [-1,1] with Y increasing upward in NDC (OpenGL-style; use OrthographicVulkan for Spark/Vulkan).
     * Z → [0,1] (Vulkan depth), linear between nearZ and farZ with farZ > nearZ.
     * Frustum planes are in the same space as the points you multiply (typically view space before projection).
     */
    static Matrix4 Orthographic(float left, float right, float bottom, float top, float nearZ, float farZ) noexcept {
        const float rl = right - left;
        const float tb = top - bottom;
        const float fn = farZ - nearZ;
        if (std::fabs(rl) < Epsilon || std::fabs(tb) < Epsilon || std::fabs(fn) < Epsilon) {
            return IdentityMatrix();
        }
        Matrix4 r = IdentityMatrix();
        r.m[0] = 2.0F / rl;
        r.m[5] = 2.0F / tb;
        r.m[10] = 1.0F / fn;
        r.m[12] = -(right + left) / rl;
        r.m[13] = -(top + bottom) / tb;
        r.m[14] = -nearZ / fn;
        return r;
    }

    /**
     * Same volume as Orthographic() but flips clip Y for Vulkan with a normal (positive height) viewport,
     * matching Matrix4::PerspectiveVulkan (negates Y scale and the Y translation term).
     */
    static Matrix4 OrthographicVulkan(float left, float right, float bottom, float top, float nearZ, float farZ) noexcept {
        Matrix4 r = Orthographic(left, right, bottom, top, nearZ, farZ);
        r.m[5] *= -1.0F;
        r.m[13] *= -1.0F;
        return r;
    }

    /** Right-handed look-at. Camera at eye, looks toward target, up is world up hint. */
    static Matrix4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) noexcept {
        const Vector3 f = (target - eye).Normalized();
        Vector3 s = Vector3::Cross(f, up);
        if (s.LengthSquared() < Epsilon) {
            s = Vector3::Cross(f, Vector3::UnitX);
            if (s.LengthSquared() < Epsilon) {
                s = Vector3::Cross(f, Vector3::UnitZ);
            }
        }
        s = s.Normalized();
        Vector3 u = Vector3::Cross(s, f);
        Matrix4 r = IdentityMatrix();
        r.m[0] = s.x;
        r.m[1] = u.x;
        r.m[2] = -f.x;
        r.m[4] = s.y;
        r.m[5] = u.y;
        r.m[6] = -f.y;
        r.m[8] = s.z;
        r.m[9] = u.z;
        r.m[10] = -f.z;
        r.m[12] = -Vector3::Dot(s, eye);
        r.m[13] = -Vector3::Dot(u, eye);
        r.m[14] = Vector3::Dot(f, eye);
        return r;
    }

    [[nodiscard]] Vector4 operator*(const Vector4& v) const noexcept {
        return {m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
                m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
                m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
                m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w};
    }

    [[nodiscard]] Matrix4 operator*(const Matrix4& b) const noexcept {
        Matrix4 r{};
        for (int c = 0; c < 4; ++c) {
            for (int row = 0; row < 4; ++row) {
                float s = 0.0F;
                for (int k = 0; k < 4; ++k) {
                    s += m[k * 4 + row] * b.m[c * 4 + k];
                }
                r.m[c * 4 + row] = s;
            }
        }
        return r;
    }

    [[nodiscard]] Vector3 TransformPoint(const Vector3& p) const noexcept {
        const Vector4 r = *this * Vector4(p, 1.0F);
        return {r.x / r.w, r.y / r.w, r.z / r.w};
    }

    [[nodiscard]] Vector3 TransformVector(const Vector3& v) const noexcept {
        const Vector4 r = *this * Vector4(v, 0.0F);
        return {r.x, r.y, r.z};
    }

    [[nodiscard]] Matrix4 Transposed() const noexcept {
        Matrix4 r{};
        for (int c = 0; c < 4; ++c) {
            for (int row = 0; row < 4; ++row) {
                r.m[c * 4 + row] = m[row * 4 + c];
            }
        }
        return r;
    }

    /** Full 4x4 inverse; false if singular. */
    [[nodiscard]] bool TryInvert(Matrix4& out) const noexcept {
        const float* a = m;
        float inv[16];

        inv[0] = a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15] + a[9] * a[7] * a[14] +
                 a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
        inv[4] = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15] - a[8] * a[7] * a[14] -
                 a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
        inv[8] = a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15] + a[8] * a[7] * a[13] +
                 a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
        inv[12] = -a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14] - a[8] * a[6] * a[13] -
                  a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
        inv[1] = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15] - a[9] * a[3] * a[14] -
                 a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
        inv[5] = a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15] + a[8] * a[3] * a[14] +
                 a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
        inv[9] = -a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15] - a[8] * a[3] * a[13] -
                 a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
        inv[13] = a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14] + a[8] * a[2] * a[13] +
                  a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
        inv[2] = a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15] + a[5] * a[3] * a[14] +
                 a[13] * a[2] * a[7] - a[13] * a[3] * a[6];
        inv[6] = -a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15] - a[4] * a[3] * a[14] -
                 a[12] * a[2] * a[7] + a[12] * a[3] * a[6];
        inv[10] = a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15] + a[4] * a[3] * a[13] +
                  a[12] * a[1] * a[7] - a[12] * a[3] * a[5];
        inv[14] = -a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14] - a[4] * a[2] * a[13] -
                  a[12] * a[1] * a[6] + a[12] * a[2] * a[5];
        inv[3] = -a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11] - a[5] * a[3] * a[10] -
                 a[9] * a[2] * a[7] + a[9] * a[3] * a[6];
        inv[7] = a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11] + a[4] * a[3] * a[10] +
                 a[8] * a[2] * a[7] - a[8] * a[3] * a[6];
        inv[11] = -a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11] - a[4] * a[3] * a[9] -
                  a[8] * a[1] * a[7] + a[8] * a[3] * a[5];
        inv[15] = a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10] + a[4] * a[2] * a[9] +
                  a[8] * a[1] * a[6] - a[8] * a[2] * a[5];

        const float det =
                a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
        if (std::fabs(det) < Epsilon) {
            return false;
        }
        const float invDet = 1.0F / det;
        for (int i = 0; i < 16; ++i) {
            out.m[i] = inv[i] * invDet;
        }
        return true;
    }

    /** Determinant of the upper-left 3x3 (rotation * scale). */
    [[nodiscard]] float DeterminantUpper3x3() const noexcept {
        return m[0] * (m[5] * m[10] - m[6] * m[9]) - m[4] * (m[1] * m[10] - m[2] * m[9]) +
               m[8] * (m[1] * m[6] - m[2] * m[5]);
    }

    /** Upper-left 3x3 (rotation * scale columns). */
    [[nodiscard]] Matrix3 Upper3x3() const noexcept {
        Matrix3 r{};
        for (int c = 0; c < 3; ++c) {
            for (int row = 0; row < 3; ++row) {
                r.m[c * 3 + row] = m[c * 4 + row];
            }
        }
        return r;
    }

    [[nodiscard]] Vector3 TranslationVector() const noexcept { return {m[12], m[13], m[14]}; }
};

inline const Matrix4 Matrix4::Identity = Matrix4::IdentityMatrix();

}  // namespace Spark
