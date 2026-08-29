#pragma once

#include "spark/editor/EditorCommandStack.hpp"

namespace Spark {

class GameObject;

namespace Editor {

/** Undoable rename of a <c>GameObject</c>. */
class SetGameObjectNameCommand final : public IEditorCommand {
public:
    SetGameObjectNameCommand(GameObject& target, Utf8String before, Utf8String after);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

private:
    GameObject* target = nullptr;
    Utf8String before{};
    Utf8String after{};
};

}  // namespace Editor
}  // namespace Spark
