#pragma once

#include "spark/math/Constants.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

/**
 * 3x3 matrix, column-major: column c stored at indices c*3 + row (row 0..2).
 * Useful for normals / 2D affine.
 */
struct Matrix3 {
    float m[9]{};  // column-major

    static const Matrix3 Identity;

    static Matrix3 IdentityMatrix() noexcept {
        Matrix3 r{};
        r.m[0] = r.m[4] = r.m[8] = 1.0F;
        return r;
    }

    static Matrix3 FromScale(const Vector3& scale) noexcept {
        Matrix3 r{};
        r.m[0] = scale.x;
        r.m[4] = scale.y;
        r.m[8] = scale.z;
        return r;
    }

    /** Columns are basis vectors (x axis, y axis, z axis). */
    static Matrix3 FromColumns(const Vector3& c0, const Vector3& c1, const Vector3& c2) noexcept {
        Matrix3 r{};
        r.m[0] = c0.x;
        r.m[1] = c0.y;
        r.m[2] = c0.z;
        r.m[3] = c1.x;
        r.m[4] = c1.y;
        r.m[5] = c1.z;
        r.m[6] = c2.x;
        r.m[7] = c2.y;
        r.m[8] = c2.z;
        return r;
    }

    [[nodiscard]] Vector3 operator*(const Vector3& v) const noexcept {
        return {m[0] * v.x + m[3] * v.y + m[6] * v.z,
                m[1] * v.x + m[4] * v.y + m[7] * v.z,
                m[2] * v.x + m[5] * v.y + m[8] * v.z};
    }

    [[nodiscard]] Matrix3 operator*(const Matrix3& b) const noexcept {
        Matrix3 r{};
        for (int col = 0; col < 3; ++col) {
            for (int row = 0; row < 3; ++row) {
                float s = 0.0F;
                for (int k = 0; k < 3; ++k) {
                    s += m[k * 3 + row] * b.m[col * 3 + k];
                }
                r.m[col * 3 + row] = s;
            }
        }
        return r;
    }

    [[nodiscard]] Matrix3 Transposed() const noexcept {
        Matrix3 r{};
        for (int c = 0; c < 3; ++c) {
            for (int row = 0; row < 3; ++row) {
                r.m[c * 3 + row] = m[row * 3 + c];
            }
        }
        return r;
    }

    /** Full inverse; returns false if singular. */
    [[nodiscard]] bool TryInvert(Matrix3& out) const noexcept {
        const float a = m[0], b = m[3], c = m[6];
        const float d = m[1], e = m[4], f = m[7];
        const float g = m[2], h = m[5], i = m[8];
        const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
        if (std::fabs(det) < Epsilon) {
            return false;
        }
        const float invDet = 1.0F / det;
        out.m[0] = (e * i - f * h) * invDet;
        out.m[1] = (c * h - b * i) * invDet;
        out.m[2] = (b * f - c * e) * invDet;
        out.m[3] = (f * g - d * i) * invDet;
        out.m[4] = (a * i - c * g) * invDet;
        out.m[5] = (c * d - a * f) * invDet;
        out.m[6] = (d * h - e * g) * invDet;
        out.m[7] = (b * g - a * h) * invDet;
        out.m[8] = (a * e - b * d) * invDet;
        return true;
    }
};

inline const Matrix3 Matrix3::Identity = Matrix3::IdentityMatrix();

}  // namespace Spark
