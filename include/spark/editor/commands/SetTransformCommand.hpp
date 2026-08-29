#pragma once

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/math/Transform.hpp"

namespace Spark {

class GameObject;

namespace Editor {

/** Undoable TRS edit on a single <c>TransformComponent</c>. */
class SetTransformCommand final : public IEditorCommand {
public:
    SetTransformCommand(GameObject& target, Transform before, Transform after);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

    [[nodiscard]] static bool NearlyEqual(const Transform& a, const Transform& b) noexcept;

private:
    GameObject* target = nullptr;
    Transform before{};
    Transform after{};
};

}  // namespace Editor
}  // namespace Spark
