#pragma once

#include "spark/core/TypeTraits.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiSkin.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark::Gui {
class GuiPaintContext;
}  // namespace Spark::Gui

namespace Spark {

class IInput;

/**
 * ECS hook for a retained-mode GUI tree: one root Widget painted in sort order with other canvases.
 * Input is routed to the frontmost canvas under the cursor; keyboard goes to the canvas that holds focus.
 */
class GuiCanvasComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GuiCanvas;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void SetSortOrder(int order) noexcept { sortOrder = order; }
    [[nodiscard]] int GetSortOrder() const noexcept { return sortOrder; }

    void SetCanvasEnabled(bool e) noexcept {
        if (canvasEnabled && !e) {
            hotWidget = nullptr;
            activePress = nullptr;
            focusWidget = nullptr;
        }
        canvasEnabled = e;
    }
    [[nodiscard]] bool IsCanvasEnabled() const noexcept { return canvasEnabled; }

    template<typename T>
        requires DerivedFrom<T, Gui::Widget>
    void SetRoot(UniquePtr<T> widget) {
        if (widget) {
            root = UniquePtr<Gui::Widget>(static_cast<Gui::Widget*>(widget.Release()));
        } else {
            root.Reset();
        }
    }
    [[nodiscard]] Gui::Widget* GetRoot() noexcept { return root.Get(); }
    [[nodiscard]] const Gui::Widget* GetRoot() const noexcept { return root.Get(); }

    void ClearTransientPointerState() noexcept;
    void ClearKeyboardFocus() noexcept { focusWidget = nullptr; }

    /** Precomputed hover from <c>ProcessGuiCanvasesInput</c> (open popups, z-order). Cleared after one <c>StepPointer</c>. */
    void SetFrameHotWidget(Gui::Widget* widget) noexcept { frameHotWidget = widget; }

    void StepPointer(const Gui::GuiFrameInput& frameInput);
    void ProcessKeyFocus(IInput& input);

    void Paint(Gui::GuiPaintContext& ctx) const;

    /** Per-canvas palette (default: `GuiTheme::ClassicMint()`). */
    void SetTheme(Gui::GuiTheme theme) noexcept { guiTheme = theme; }
    [[nodiscard]] const Gui::GuiTheme& GetTheme() const noexcept { return guiTheme; }

    void SetSkin(SharedPtr<Gui::GuiSkin> skin) noexcept { guiSkin = MoveTemp(skin); }
    [[nodiscard]] const SharedPtr<Gui::GuiSkin>& GetSkin() const noexcept { return guiSkin; }
    [[nodiscard]] Gui::GuiSkin* GetSkinMutable() noexcept { return guiSkin.Get(); }

    void SetLayoutMetrics(Gui::GuiLayoutMetrics metrics) noexcept { layoutMetrics = metrics; }
    [[nodiscard]] const Gui::GuiLayoutMetrics& GetLayoutMetrics() const noexcept { return layoutMetrics; }

    /** When true, only this canvas receives pointer/keyboard (modal dialog). */
    void SetModalInputCapture(bool capture) noexcept { modalInputCapture = capture; }
    [[nodiscard]] bool GetModalInputCapture() const noexcept { return modalInputCapture; }

    [[nodiscard]] Gui::Widget* GetHotWidget() const noexcept { return hotWidget; }
    [[nodiscard]] Gui::Widget* GetActivePressWidget() const noexcept { return activePress; }
    [[nodiscard]] Gui::Widget* GetFocusWidget() const noexcept { return focusWidget; }

    /** Last pointer frame passed to <c>StepPointer</c> (modifiers for multi-select, etc.). */
    [[nodiscard]] const Gui::GuiFrameInput& GetLastFrameInput() const noexcept { return lastFrameInput; }

private:
    int sortOrder = 0;
    bool canvasEnabled = true;
    UniquePtr<Gui::Widget> root{};
    Gui::GuiTheme guiTheme{Gui::GuiTheme::ClassicMint()};
    SharedPtr<Gui::GuiSkin> guiSkin{};
    Gui::GuiLayoutMetrics layoutMetrics{Gui::GuiLayoutMetrics::Default()};

    Gui::Widget* hotWidget = nullptr;
    Gui::Widget* activePress = nullptr;
    Gui::Widget* focusWidget = nullptr;
    Gui::Widget* frameHotWidget = nullptr;
    Gui::GuiFrameInput lastFrameInput{};
    bool modalInputCapture = false;
};

}  // namespace Spark
