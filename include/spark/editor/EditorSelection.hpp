#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

namespace Spark {

class GameObject;

namespace Editor {

/**
 * Engine-level selection set for the editor (replaces per-demo selectedObject).
 * Single primary selection + optional multi-select extension later.
 */
class EditorSelection {
public:
    void Clear() noexcept;
    void SetPrimary(GameObject* object) noexcept;
    [[nodiscard]] GameObject* GetPrimary() const noexcept { return primary; }
    [[nodiscard]] bool IsSelected(const GameObject* object) const noexcept;

    using ChangedCallback = void (*)(void* userData);
    void SetOnChanged(ChangedCallback callback, void* userData) noexcept;

private:
    void NotifyChanged() noexcept;

    GameObject* primary = nullptr;
    ChangedCallback onChanged = nullptr;
    void* onChangedUserData = nullptr;
};

}  // namespace Editor
}  // namespace Spark
