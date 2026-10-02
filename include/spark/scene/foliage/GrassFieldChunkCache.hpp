#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/scene/foliage/FoliageInstanceRecord.hpp"
#include "spark/scene/foliage/GrassChunkCoordinate.hpp"

namespace Spark {

/** Cached instance lists per chunk coordinate (F2-02). */
class GrassFieldChunkCache {
public:
    class ChunkPayload {
    public:
        [[nodiscard]] const Array<FoliageInstanceRecord>& GetInstances() const noexcept { return instances; }
        Array<FoliageInstanceRecord>& EditInstances() noexcept { return instances; }

        [[nodiscard]] bool HasScatterForChunk() const noexcept { return scatterComplete; }
        void MarkScatterComplete() noexcept { scatterComplete = true; }

    private:
        Array<FoliageInstanceRecord> instances{};
        bool scatterComplete = false;
    };

    [[nodiscard]] ChunkPayload* Find(const GrassChunkCoordinate& coord) noexcept;
    [[nodiscard]] const ChunkPayload* Find(const GrassChunkCoordinate& coord) const noexcept;

    ChunkPayload& FindOrInsert(const GrassChunkCoordinate& coord);

    void RemoveExcept(const Array<GrassChunkCoordinate>& keepList);

    void Clear() noexcept;

private:
    HashMap<GrassChunkCoordinate, ChunkPayload> storage{};
    Array<GrassChunkCoordinate> cachedKeys{};
};

}  // namespace Spark
