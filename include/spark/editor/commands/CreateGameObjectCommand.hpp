#pragma once

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/core/Utf8String.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class SceneEditorContentModel;

namespace Editor {

/** Undoable creation of an empty <c>GameObject</c> with a <c>TransformComponent</c>. */
class CreateGameObjectCommand final : public IEditorCommand {
public:
    CreateGameObjectCommand(
            GameWorld& world,
            SceneEditorContentModel& content,
            Utf8String name,
            GameObject* parent);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

    [[nodiscard]] GameObject* GetCreatedObject() const noexcept { return created; }

private:
    void CreateObject();
    void DestroyCreated();

    GameWorld* world = nullptr;
    SceneEditorContentModel* content = nullptr;
    Utf8String objectName{};
    std::uint64_t parentObjectId = 0;
    GameObject* created = nullptr;
};

}  // namespace Editor
}  // namespace Spark
