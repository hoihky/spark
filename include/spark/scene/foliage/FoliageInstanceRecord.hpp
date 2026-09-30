#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"

namespace Spark {

/** One CPU-side foliage instance before GPU packing (F1). */
class FoliageInstanceRecord {
public:
    FoliageInstanceRecord() = default;

    void SetModelMatrix(const Matrix4& worldFromLocal) noexcept { model = worldFromLocal; }
    [[nodiscard]] const Matrix4& GetModelMatrix() const noexcept { return model; }

    void SetTint(const Vector4& value) noexcept { tint = value; }
    [[nodiscard]] Vector4 GetTint() const noexcept { return tint; }

    void SetWindPhase(float radians) noexcept { windPhase = radians; }
    [[nodiscard]] float GetWindPhase() const noexcept { return windPhase; }

private:
    Matrix4 model = Matrix4::Identity;
    Vector4 tint{1.0F, 1.0F, 1.0F, 1.0F};
    float windPhase = 0.0F;
};

}  // namespace Spark
