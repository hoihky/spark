#pragma once

#include "spark/editor/EditorCommandStack.hpp"

namespace Spark {

class GameObject;
class GameWorld;

namespace Editor {

/** Undoable parent change on a single <c>GameObject</c>. */
class SetGameObjectParentCommand final : public IEditorCommand {
public:
    SetGameObjectParentCommand(GameWorld& world, GameObject& child, GameObject* newParent);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

private:
    void ApplyParent(std::uint64_t parentId);

    GameWorld* world = nullptr;
    std::uint64_t childId = 0;
    std::uint64_t parentBeforeId = 0;
    std::uint64_t parentAfterId = 0;
    Utf8String childName{};
};

}  // namespace Editor
}  // namespace Spark
