#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

namespace Spark {

class TilemapComponent;

/** Canonical sparse tilemap payload shared by scene snapshot handlers. */
class TilemapComponentSnapshot {
public:
    [[nodiscard]] static bool TryCapture(
            const TilemapComponent& tilemap,
            const GameObject& owner,
            const SceneCaptureContext& ctx,
            Utf8String& outPayload);

    [[nodiscard]] static bool TryRestore(
            GameObject& owner,
            const char* payload,
            GameWorld& world,
            const SceneApplyContext& ctx);
};

}  // namespace Spark
