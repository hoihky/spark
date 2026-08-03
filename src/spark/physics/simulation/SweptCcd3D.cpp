#include "spark/physics/simulation/SweptCcd3D.hpp"

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/colliders/DynamicCollider3D.hpp"
#include "spark/physics/SpatialHashGrid3D.hpp"

#include <algorithm>

namespace Spark {

bool SweptCcd3D::AnyStaticOverlapForDynamic(
        const DynamicCollider3D& collider,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    broadPhase.QueryUniquePayloadIndices(collider.GetBounds(), scratch);
    for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
        const std::uint32_t idx = scratch[i];
        if (idx >= colliders.GetSize()) {
            continue;
        }
        if (collider.OverlapsStatic(colliders[idx])) {
            return true;
        }
    }
    return false;
}

float SweptCcd3D::ComputeTranslationLambdaAgainstStatics(
        const DynamicCollider3D& colliderAtStart,
        const Vector3& track0,
        const Vector3& track1,
        const int binaryIterations,
        const Array<Collider3D>& colliders,
        SpatialHashGrid3D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    if (binaryIterations <= 0) {
        return 1.0F;
    }

    DynamicCollider3D colliderEnd = DynamicCollider3D::FromLegacySnapshot(colliderAtStart.ToLegacySnapshot());
    const Vector3 endDelta{track1.x - track0.x, track1.y - track0.y, track1.z - track0.z};
    colliderEnd.Translate(endDelta);
    if (!AnyStaticOverlapForDynamic(colliderEnd, colliders, broadPhase, scratch)) {
        return 1.0F;
    }
    if (!AnyStaticOverlapForDynamic(colliderAtStart, colliders, broadPhase, scratch)) {
        float lo = 0.0F;
        float hi = 1.0F;
        for (int k = 0; k < binaryIterations; ++k) {
            const float mid = 0.5F * (lo + hi);
            DynamicCollider3D colliderMid = DynamicCollider3D::FromLegacySnapshot(colliderAtStart.ToLegacySnapshot());
            colliderMid.Translate({endDelta.x * mid, endDelta.y * mid, endDelta.z * mid});
            if (AnyStaticOverlapForDynamic(colliderMid, colliders, broadPhase, scratch)) {
                hi = mid;
            } else {
                lo = mid;
            }
        }
        return std::max(0.0F, lo * 0.999F);
    }
    return 1.0F;
}

}  // namespace Spark
