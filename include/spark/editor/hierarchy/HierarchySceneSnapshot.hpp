#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

namespace Spark {

class GameObject;
class GameWorld;

namespace Editor {

/** Result of restoring a captured hierarchy subtree. */
struct HierarchyRestoreResult {
    GameObject* root = nullptr;
    HashMap<std::uint64_t, GameObject*> idToObject{};
};

/**
 * Captures and restores ECS subtrees via <c>SceneSerializer</c> (Memento for delete/duplicate undo).
 */
class HierarchySceneSnapshot final {
public:
  [[nodiscard]] static bool IsEditorChrome(const GameObject* object) noexcept;

  [[nodiscard]] static SceneDocument CaptureSubtree(
          const GameWorld& world,
          const GameObject& root,
          const SceneCaptureContext& ctx);

  [[nodiscard]] static bool RestoreSubtree(
          const SceneDocument& document,
          GameWorld& world,
          const SceneApplyContext& ctx,
          std::uint64_t rootEntityId,
          HierarchyRestoreResult& out);

  [[nodiscard]] static std::uint64_t FindRootEntityId(
          const SceneDocument& document,
          const GameObject& root) noexcept;

  static void CollectSubtree(const GameObject& root, Array<const GameObject*>& out);
};

}  // namespace Editor
}  // namespace Spark
