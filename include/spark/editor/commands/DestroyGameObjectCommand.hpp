#pragma once

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/editor/hierarchy/HierarchySceneSnapshot.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class SceneEditorContentModel;

namespace Editor {

class EditorSelection;

/** Undoable deletion of a <c>GameObject</c> subtree (Memento via <c>SceneDocument</c>). */
class DestroyGameObjectCommand final : public IEditorCommand {
public:
    DestroyGameObjectCommand(
            GameWorld& world,
            SceneEditorContentModel& content,
            EditorSelection& selection,
            GameObject& target);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

private:
    void DestroyTarget();
    void RestoreTarget();

    GameWorld* world = nullptr;
    SceneEditorContentModel* content = nullptr;
    EditorSelection* selection = nullptr;
    SceneDocument snapshot{};
    SceneCaptureContext captureCtx{};
    std::uint64_t rootEntityId = 0;
    std::uint64_t initialLiveId = 0;
    std::uint64_t restoredLiveId = 0;
    std::uint64_t selectedObjectId = 0;
    Utf8String targetName{};
};

}  // namespace Editor
}  // namespace Spark
