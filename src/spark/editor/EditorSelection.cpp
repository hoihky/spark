#include "spark/editor/EditorSelection.hpp"

namespace Spark::Editor {

void EditorSelection::Clear() noexcept {
    if (primary == nullptr) {
        return;
    }
    primary = nullptr;
    NotifyChanged();
}

void EditorSelection::SetPrimary(GameObject* const object) noexcept {
    if (primary == object) {
        return;
    }
    primary = object;
    NotifyChanged();
}

bool EditorSelection::IsSelected(const GameObject* const object) const noexcept {
    return object != nullptr && primary == object;
}

void EditorSelection::SetOnChanged(const ChangedCallback callback, void* const userData) noexcept {
    onChanged = callback;
    onChangedUserData = userData;
}

void EditorSelection::NotifyChanged() noexcept {
    if (onChanged != nullptr) {
        onChanged(onChangedUserData);
    }
}

}  // namespace Spark::Editor
