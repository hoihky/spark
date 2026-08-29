#include "spark/editor/EditorCommandStack.hpp"

namespace Spark::Editor {

void EditorCommandStack::Execute(UniquePtr<IEditorCommand> command) {
    if (!command) {
        return;
    }
    command->Redo();
    undoStack.PushBack(MoveTemp(command));
    redoStack.Clear();
    TrimUndoStack();
}

void EditorCommandStack::Record(UniquePtr<IEditorCommand> command) {
    if (!command) {
        return;
    }
    undoStack.PushBack(MoveTemp(command));
    redoStack.Clear();
    TrimUndoStack();
}

bool EditorCommandStack::TryUndo() {
    if (undoStack.IsEmpty()) {
        return false;
    }
    UniquePtr<IEditorCommand> command = MoveTemp(undoStack.GetLast());
    undoStack.PopBack();
    command->Undo();
    redoStack.PushBack(MoveTemp(command));
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
