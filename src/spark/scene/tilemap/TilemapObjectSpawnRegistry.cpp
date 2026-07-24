#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"

#include "spark/core/HashMap.hpp"
#include "spark/scene/GameWorldAssetCache.hpp"

namespace Spark {

namespace {

struct RegistryState {
    HashMap<Utf8String, TilemapObjectSpawnFn, Detail::Utf8StringHasher> handlers{};
};

RegistryState& State() noexcept {
    static RegistryState s{};
    return s;
}

}  // namespace

void TilemapObjectSpawnRegistry::Register(const char* typeId, const TilemapObjectSpawnFn spawnFn) noexcept {
    if (typeId == nullptr || spawnFn == nullptr) {
        return;
    }
    State().handlers.Add(Utf8String(typeId), spawnFn);
}

void TilemapObjectSpawnRegistry::Unregister(const char* typeId) noexcept {
    if (typeId == nullptr) {
        return;
    }
    State().handlers.Remove(Utf8String(typeId));
}

TilemapObjectSpawnFn TilemapObjectSpawnRegistry::Find(const Utf8String& typeId) noexcept {
    if (const TilemapObjectSpawnFn* fn = State().handlers.Find(typeId); fn != nullptr) {
        return *fn;
    }
    return nullptr;
}

void TilemapObjectSpawnRegistry::Clear() noexcept {
    State().handlers.Clear();
}

}  // namespace Spark
