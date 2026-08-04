#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorContext.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/IUiElement.hpp"

namespace Spark::Editor {

/**
 * Dock panel contract — one panel per editor region (Hierarchy, Inspector, …).
 * Implementations own their widget subtree; EditorApplication arranges roots.
 */
class IEditorPanel {
public:
    virtual ~IEditorPanel() = default;

    [[nodiscard]] virtual Utf8String GetPanelId() const = 0;
    [[nodiscard]] virtual Utf8String GetDisplayName() const = 0;
    [[nodiscard]] virtual Ui::IUiElement* GetRootElement() noexcept = 0;

    virtual void OnAttach(EditorContext& /*ctx*/) {}
    virtual void OnDetach() {}
    virtual void OnTick(const FrameTiming& /*timing*/, EditorContext& /*ctx*/) {}

    [[nodiscard]] virtual UniquePtr<Ui::IUiElement> ReleaseRootElement() {
        return UniquePtr<Ui::IUiElement>{};
    }
};

}  // namespace Spark::Editor
