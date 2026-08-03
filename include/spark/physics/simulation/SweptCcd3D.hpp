#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/colliders/DynamicCollider3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"

namespace Spark {

/** Swept broad-phase CCD along a translation segment against static colliders. */
class SweptCcd3D {
public:
    [[nodiscard]] static bool AnyStaticOverlapForDynamic(
            const DynamicCollider3D& collider,
            const Array<Collider3D>& colliders,
            SpatialHashGrid3D& broadPhase,
            Array<std::uint32_t>& scratch) noexcept;

    [[nodiscard]] static float ComputeTranslationLambdaAgainstStatics(
            const DynamicCollider3D& colliderAtStart,
            const Vector3& track0,
            const Vector3& track1,
            int binaryIterations,
            const Array<Collider3D>& colliders,
            SpatialHashGrid3D& broadPhase,
            Array<std::uint32_t>& scratch) noexcept;
};

}  // namespace Spark
