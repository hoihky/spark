#include "spark/scene/foliage/GrassFieldStreamingPolicy.hpp"

namespace Spark {

void GrassFieldStreamingPolicy::Reset() noexcept {
    hasScatterCenter = false;
}

bool GrassFieldStreamingPolicy::NeedsChunkScatterRefresh(const GrassChunkCoordinate& centerChunk) const noexcept {
    return !hasScatterCenter || !(centerChunk == lastScatterCenterChunk);
}

void GrassFieldStreamingPolicy::NotifyChunkScatterRefreshed(const GrassChunkCoordinate& centerChunk) noexcept {
    lastScatterCenterChunk = centerChunk;
    hasScatterCenter = true;
}

}  // namespace Spark
