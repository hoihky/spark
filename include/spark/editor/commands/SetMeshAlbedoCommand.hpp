#pragma once

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;

namespace Editor {

/** Undoable edit of <c>MeshComponent</c> albedo tint. */
class SetMeshAlbedoCommand final : public IEditorCommand {
public:
    SetMeshAlbedoCommand(GameObject& target, Vector3 before, Vector3 after);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

    [[nodiscard]] static bool NearlyEqual(const Vector3& a, const Vector3& b) noexcept;

private:
    GameObject* target = nullptr;
    Vector3 before{};
    Vector3 after{};
};

}  // namespace Editor
}  // namespace Spark
