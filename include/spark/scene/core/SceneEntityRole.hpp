#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

namespace Spark {

class GameWorld;

/** High-level role of a serialized entity for editor/runtime binding. */
enum class SceneEntityRole : std::uint8_t {
    None = 0,
    Mesh,
    Light,
    SpawnPoint,
};

struct SceneEntityRoleInfo {
    SceneEntityRole role = SceneEntityRole::None;
    Utf8String meshAssetPath{};
};

/** Classifies <c>spark_scene_v4</c> entity records by component tags (Strategy). */
class SceneEntityRoleClassifier final {
public:
    [[nodiscard]] static SceneEntityRoleInfo Classify(const EntityRecord& entity) noexcept;
    static void SortObjectsById(Array<GameObject*>& objects) noexcept;
    [[nodiscard]] static Array<GameObject*> CollectInstanceObjects(
            const GameWorld& world,
            SceneInstanceId instanceId) noexcept;
};

}  // namespace Spark
