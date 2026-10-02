#pragma once

#include "spark/scene/foliage/GrassChunkCoordinate.hpp"

namespace Spark {

/**
 * Chunk scatter runs only when the camera-centered chunk index changes.
 * Instance merge runs from <c>PrepareForRender</c> (throttled by view movement / chunk change).
 */
class GrassFieldStreamingPolicy {
public:
    void Reset() noexcept;

    [[nodiscard]] bool NeedsChunkScatterRefresh(const GrassChunkCoordinate& centerChunk) const noexcept;

    void NotifyChunkScatterRefreshed(const GrassChunkCoordinate& centerChunk) noexcept;

private:
    GrassChunkCoordinate lastScatterCenterChunk{};
    bool hasScatterCenter = false;
};

}  // namespace Spark
