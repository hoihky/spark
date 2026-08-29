#pragma once

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class SceneEditorContentModel;

namespace Editor {

class EditorSelection;

/** Undoable duplicate of a <c>GameObject</c> subtree. */
class DuplicateGameObjectCommand final : public IEditorCommand {
public:
    DuplicateGameObjectCommand(
            GameWorld& world,
            SceneEditorContentModel& content,
            EditorSelection& selection,
            GameObject& source);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

private:
    void CreateDuplicate();
    void DestroyDuplicate();

    GameWorld* world = nullptr;
    SceneEditorContentModel* content = nullptr;
    EditorSelection* selection = nullptr;
    SceneDocument templateDocument{};
    SceneCaptureContext captureCtx{};
    std::uint64_t rootEntityId = 0;
    std::uint64_t parentObjectId = 0;
    std::uint64_t duplicateLiveId = 0;
    Utf8String sourceName{};
};

}  // namespace Editor
}  // namespace Spark
