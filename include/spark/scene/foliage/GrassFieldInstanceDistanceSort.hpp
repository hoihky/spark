#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/foliage/FoliageInstanceRecord.hpp"

namespace Spark {

/** Nearest-instance selection for grass merge budgets (no full-array sort). */
class GrassFieldInstanceDistanceSort {
public:
    static std::size_t AppendNearestWithinView(
            const Array<FoliageInstanceRecord>& source,
            const Vector3& viewWorld,
            float maxViewDistanceSq,
            std::uint32_t maxCount,
            Array<FoliageInstanceRecord>& dest,
            Array<bool>& pickedScratch) noexcept;
};

}  // namespace Spark
