#include "spark/editor/EditorSelection.hpp"

namespace Spark::Editor {

void EditorSelection::Clear() noexcept {
    if (primary_ == nullptr) {
        return;
    }
    primary_ = nullptr;
    NotifyChanged();
}

void EditorSelection::SetPrimary(GameObject* const object) noexcept {
    if (primary_ == object) {
        return;
    }
    primary_ = object;
    NotifyChanged();
}

bool EditorSelection::IsSelected(const GameObject* const object) const noexcept {
    return object != nullptr && primary_ == object;
}

void EditorSelection::SetOnChanged(const ChangedCallback callback, void* const userData) noexcept {
    onChanged_ = callback;
    onChangedUserData_ = userData;
}

void EditorSelection::NotifyChanged() noexcept {
    if (onChanged_ != nullptr) {
        onChanged_(onChangedUserData_);
    }
}

}  // namespace Spark::Editor
