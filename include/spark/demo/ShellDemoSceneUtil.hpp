#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/core/Array.hpp"

namespace Spark {

Quaternion QuaternionFromRotationColumns(
        const Vector3& col0, const Vector3& col1, const Vector3& col2) noexcept;

int DrawSortKey(const SceneDrawItem& it);

void StableSortDrawItems(Array<SceneDrawItem>& items);

[[nodiscard]] bool TerrainScreenToWorldRay(
        int fbW,
        int fbH,
        float px,
        float py,
        const Matrix4& invViewProj,
        Vector3& outOrigin,
        Vector3& outDir);

}  // namespace Spark
