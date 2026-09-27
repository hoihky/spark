#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"

namespace Spark {

class GameObject;
class GameWorld;

/**
 * Strategy hook for map object types. Games register spawn handlers per <c>typeId</c>;
 * <c>TilemapObjectSpawnComponent</c> invokes them when markers load.
 */
using TilemapObjectSpawnFn = GameObject* (*)(GameWorld& world,
                                            GameObject& mapOwner,
                                            const TilemapObjectMarker& marker,
                                            const TilemapGridFrame& frame);

/** Type-id → spawn function registry (use <c>Default()</c> for the process-wide table). */
class TilemapObjectSpawnRegistry final {
public:
    [[nodiscard]] static TilemapObjectSpawnRegistry& Default() noexcept;

    void Register(const char* typeId, TilemapObjectSpawnFn spawnFn) noexcept;
    void Unregister(const char* typeId) noexcept;
    [[nodiscard]] TilemapObjectSpawnFn Find(const Utf8String& typeId) const noexcept;
    void Clear() noexcept;

private:
    TilemapObjectSpawnRegistry() = default;

    struct HandlerMap;
    HandlerMap& entries() noexcept;
    const HandlerMap& entries() const noexcept;
};

}  // namespace Spark
