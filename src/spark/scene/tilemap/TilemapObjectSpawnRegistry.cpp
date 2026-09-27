#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"

#include "spark/core/HashMap.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"

namespace Spark {

struct TilemapObjectSpawnRegistry::HandlerMap {
    HashMap<Utf8String, TilemapObjectSpawnFn, Detail::Utf8StringHasher> handlers{};
};

TilemapObjectSpawnRegistry& TilemapObjectSpawnRegistry::Default() noexcept {
    static TilemapObjectSpawnRegistry registry{};
    return registry;
}

TilemapObjectSpawnRegistry::HandlerMap& TilemapObjectSpawnRegistry::entries() noexcept {
    static HandlerMap map{};
    return map;
}

const TilemapObjectSpawnRegistry::HandlerMap& TilemapObjectSpawnRegistry::entries() const noexcept {
    return const_cast<TilemapObjectSpawnRegistry*>(this)->entries();
}

void TilemapObjectSpawnRegistry::Register(const char* typeId, const TilemapObjectSpawnFn spawnFn) noexcept {
    if (typeId == nullptr || spawnFn == nullptr) {
        return;
    }
    entries().handlers.Add(Utf8String(typeId), spawnFn);
}

void TilemapObjectSpawnRegistry::Unregister(const char* typeId) noexcept {
    if (typeId == nullptr) {
        return;
    }
    entries().handlers.Remove(Utf8String(typeId));
}

TilemapObjectSpawnFn TilemapObjectSpawnRegistry::Find(const Utf8String& typeId) const noexcept {
    if (const TilemapObjectSpawnFn* fn = entries().handlers.Find(typeId); fn != nullptr) {
        return *fn;
    }
    return nullptr;
}

void TilemapObjectSpawnRegistry::Clear() noexcept {
    entries().handlers.Clear();
}

}  // namespace Spark
