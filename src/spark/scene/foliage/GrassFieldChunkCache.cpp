#include "spark/scene/foliage/GrassFieldChunkCache.hpp"

namespace Spark {

namespace {

bool ContainsCoord(const Array<GrassChunkCoordinate>& list, const GrassChunkCoordinate& coord) noexcept {
    for (std::size_t i = 0; i < list.GetSize(); ++i) {
        if (list[i] == coord) {
            return true;
        }
    }
    return false;
}

}  // namespace

GrassFieldChunkCache::ChunkPayload* GrassFieldChunkCache::Find(const GrassChunkCoordinate& coord) noexcept {
    return storage.Find(coord);
}

const GrassFieldChunkCache::ChunkPayload* GrassFieldChunkCache::Find(
        const GrassChunkCoordinate& coord) const noexcept {
    return storage.Find(coord);
}

GrassFieldChunkCache::ChunkPayload& GrassFieldChunkCache::FindOrInsert(const GrassChunkCoordinate& coord) {
    if (ChunkPayload* existing = storage.Find(coord)) {
        return *existing;
    }
    ChunkPayload fresh{};
    storage.Add(coord, fresh);
    cachedKeys.PushBack(coord);
    return *storage.Find(coord);
}

void GrassFieldChunkCache::RemoveExcept(const Array<GrassChunkCoordinate>& keepList) {
    for (std::size_t i = 0; i < cachedKeys.GetSize();) {
        const GrassChunkCoordinate key = cachedKeys[i];
        if (!ContainsCoord(keepList, key)) {
            storage.Remove(key);
            cachedKeys.RemoveAt(i);
        } else {
            ++i;
        }
    }
}

void GrassFieldChunkCache::Clear() noexcept {
    storage.Clear();
    cachedKeys.Clear();
}

}  // namespace Spark
