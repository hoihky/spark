#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"

namespace Spark {

class GameObject;
class GameWorld;

/**
 * Strategy hook for map object types (F2). Games register spawn functions per <c>typeId</c>;
 * <c>TilemapObjectSpawnComponent</c> invokes them when markers load.
 */
using TilemapObjectSpawnFn = GameObject* (*)(GameWorld& world,
                                            GameObject& mapOwner,
                                            const TilemapObjectMarker& marker,
                                            const TilemapGridFrame& frame);

/** Global type-id → spawn function table (process lifetime). */
class TilemapObjectSpawnRegistry final {
public:
    static void Register(const char* typeId, TilemapObjectSpawnFn spawnFn) noexcept;
    static void Unregister(const char* typeId) noexcept;
    [[nodiscard]] static TilemapObjectSpawnFn Find(const Utf8String& typeId) noexcept;
    static void Clear() noexcept;

private:
    TilemapObjectSpawnRegistry() = delete;
};

}  // namespace Spark
