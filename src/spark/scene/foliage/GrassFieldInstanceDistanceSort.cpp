#include "spark/scene/foliage/GrassFieldInstanceDistanceSort.hpp"

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

namespace {

float HorizontalDistanceSq(const Matrix4& model, const Vector3& viewWorld) noexcept {
    const Vector3 worldPos = model.TransformPoint(Vector3::Zero);
    const float dx = worldPos.x - viewWorld.x;
    const float dz = worldPos.z - viewWorld.z;
    return dx * dx + dz * dz;
}

}  // namespace

std::size_t GrassFieldInstanceDistanceSort::AppendNearestWithinView(
        const Array<FoliageInstanceRecord>& source,
        const Vector3& viewWorld,
        const float maxViewDistanceSq,
        const std::uint32_t maxCount,
        Array<FoliageInstanceRecord>& dest,
        Array<bool>& pickedScratch) noexcept {
    if (maxCount == 0U || source.IsEmpty()) {
        return 0U;
    }

    const std::size_t n = source.GetSize();
    if (pickedScratch.GetSize() < n) {
        pickedScratch.Resize(n);
    }
    for (std::size_t i = 0U; i < n; ++i) {
        pickedScratch[i] = false;
    }

    std::size_t added = 0U;
    for (std::uint32_t pick = 0U; pick < maxCount; ++pick) {
        std::size_t bestIndex = n;
        float bestDist = maxViewDistanceSq + 1.0F;
        for (std::size_t i = 0U; i < n; ++i) {
            if (pickedScratch[i]) {
                continue;
            }
            const float dist = HorizontalDistanceSq(source[i].GetModelMatrix(), viewWorld);
            if (dist > maxViewDistanceSq) {
                continue;
            }
            if (dist < bestDist) {
                bestDist = dist;
                bestIndex = i;
            }
        }
        if (bestIndex >= n) {
            break;
        }
        pickedScratch[bestIndex] = true;
        dest.PushBack(source[bestIndex]);
        ++added;
    }
    return added;
}

}  // namespace Spark
