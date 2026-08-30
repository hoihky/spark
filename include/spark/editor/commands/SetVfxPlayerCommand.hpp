#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorCommandStack.hpp"

namespace Spark {

class GameObject;

namespace Editor {

struct VfxPlayerState {
    Utf8String assetKey{};
    bool playOnStart = false;
    bool playOnStartOnce = false;
};

/** Undoable edit of <c>VfxPlayerComponent</c> inspector fields. */
class SetVfxPlayerCommand final : public IEditorCommand {
public:
    SetVfxPlayerCommand(GameObject& target, VfxPlayerState before, VfxPlayerState after);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

    [[nodiscard]] static bool NearlyEqual(const VfxPlayerState& a, const VfxPlayerState& b) noexcept;
    [[nodiscard]] static VfxPlayerState Capture(GameObject& target);

private:
    static void Apply(GameObject& target, const VfxPlayerState& state);

    GameObject* target = nullptr;
    VfxPlayerState before{};
    VfxPlayerState after{};
};

}  // namespace Editor
}  // namespace Spark
