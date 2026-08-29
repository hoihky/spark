#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

namespace Spark {

class GameObject;
class GameWorld;

/** Captures and applies per-node mesh/material overrides for expanded glTF prefab instances. */
namespace GltfInstanceOverrides {

/** Appends v2 override records for descendants tagged with <c>GltfInstanceNodeComponent</c>. */
void AppendCapturedOverrides(
        const GameObject& prefabRoot,
        const SceneCaptureContext& ctx,
        Utf8String& payload);

/** Applies override records from a v2 <c>gltf_scene</c> payload cursor (after the glTF path). */
void ApplyFromPayloadCursor(
        GameObject& prefabRoot,
        const char* cursor,
        GameWorld& world,
        const SceneApplyContext& ctx);

}  // namespace GltfInstanceOverrides

}  // namespace Spark
