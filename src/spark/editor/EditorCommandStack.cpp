#include "spark/editor/EditorCommandStack.hpp"

namespace Spark::Editor {

void EditorCommandStack::SetOnChanged(const ChangedCallback callback, void* const userDataIn) noexcept {
    onChanged = callback;
    onChangedUserData = userDataIn;
}

void EditorCommandStack::NotifyChanged() noexcept {
    if (onChanged != nullptr) {
        onChanged(onChangedUserData);
    }
}

void EditorCommandStack::Execute(UniquePtr<IEditorCommand> command) {
    if (!command) {
        return;
    }
    command->Redo();
    undoStack.PushBack(MoveTemp(command));
    redoStack.Clear();
    TrimUndoStack();
    NotifyChanged();
}

void EditorCommandStack::Record(UniquePtr<IEditorCommand> command) {
    if (!command) {
        return;
    }
    undoStack.PushBack(MoveTemp(command));
    redoStack.Clear();
    TrimUndoStack();
    NotifyChanged();
}

bool EditorCommandStack::TryUndo() {
    if (undoStack.IsEmpty()) {
        return false;
    }
    UniquePtr<IEditorCommand> command = MoveTemp(undoStack.GetLast());
    undoStack.PopBack();
    command->Undo();
    redoStack.PushBack(MoveTemp(command));
    NotifyChanged();
    return true;
}

bool EditorCommandStack::TryRedo() {
    if (redoStack.IsEmpty()) {
        return false;
    }
    UniquePtr<IEditorCommand> command = MoveTemp(redoStack.GetLast());
    redoStack.PopBack();
    command->Redo();
    undoStack.PushBack(MoveTemp(command));
    NotifyChanged();
    return true;
}

void EditorCommandStack::Clear() noexcept {
    undoStack.Clear();
    redoStack.Clear();
}

void EditorCommandStack::TrimUndoStack() noexcept {
    while (undoStack.GetSize() > kMaxDepth) {
        undoStack.RemoveAt(0);
    }
}

}  // namespace Spark::Editor
