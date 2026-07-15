#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark::Gui {

struct MenuBarItem {
    Utf8String label{};
    Array<Utf8String> dropdownEntries{};
    std::function<void(int dropdownIndex)> onDropdownSelect{};
    std::function<void()> onActivate{};
};

/** Horizontal row of menu buttons; entries with dropdowns open a popup list (like a simplified menu bar). */
class MenuBar final : public Widget {
public:
    void SetItems(Array<MenuBarItem> menuItems) { items = Spark::MoveTemp(menuItems); }
    void SetBarHeight(float h) noexcept { barHeight = h; }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

    void ClosePopup() noexcept;
    void DismissPopupUnlessHit(float mouseX, float mouseY, const Widget* hitWidget) noexcept;
    [[nodiscard]] bool IsPopupOpen() const noexcept { return openMenuIndex >= 0; }

private:
    Array<MenuBarItem> items{};
    float barHeight = 34.0F;
    int openMenuIndex = -1;
    Rect popupRect{};
};

}  // namespace Spark::Gui
