#pragma once

#include "spark/math/Matrix4.hpp"

namespace Spark {

/** One CPU-side rigid mesh instance (world transform) before GPU packing. */
class MeshInstanceRecord {
public:
    MeshInstanceRecord() = default;

    void SetModelMatrix(const Matrix4& worldFromLocal) noexcept { model = worldFromLocal; }
    [[nodiscard]] const Matrix4& GetModelMatrix() const noexcept { return model; }

private:
    Matrix4 model = Matrix4::Identity;
};

}  // namespace Spark