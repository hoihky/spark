#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark::Editor {

/**
 * Reversible editor action (Command pattern).
 * <c>Redo</c> applies the new state; <c>Undo</c> restores the prior state.
 */
class IEditorCommand {
public:
    virtual ~IEditorCommand() = default;

    virtual void Undo() = 0;
    virtual void Redo() = 0;
    [[nodiscard]] virtual Utf8String GetDescription() const = 0;
};

/**
 * Undo/redo stack for editor mutations. New execute clears the redo branch.
 */
class EditorCommandStack final {
public:
    /** Applies <c>command</c> via <c>Redo</c> and pushes onto the undo stack. */
    void Execute(UniquePtr<IEditorCommand> command);

    /** Records a command whose <c>Redo</c> state is already applied (e.g. end of gizmo drag). */
    void Record(UniquePtr<IEditorCommand> command);

    [[nodiscard]] bool TryUndo();
    [[nodiscard]] bool TryRedo();
    void Clear() noexcept;

    [[nodiscard]] bool CanUndo() const noexcept { return !undoStack.IsEmpty(); }
    [[nodiscard]] bool CanRedo() const noexcept { return !redoStack.IsEmpty(); }

    using ChangedCallback = void (*)(void* userData);
    void SetOnChanged(ChangedCallback callback, void* userDataIn) noexcept;

private:
    static constexpr std::size_t kMaxDepth = 64;

    void TrimUndoStack() noexcept;
    void NotifyChanged() noexcept;

    ChangedCallback onChanged = nullptr;
    void* onChangedUserData = nullptr;

    Array<UniquePtr<IEditorCommand>> undoStack{};
    Array<UniquePtr<IEditorCommand>> redoStack{};
};

}  // namespace Spark::Editor
