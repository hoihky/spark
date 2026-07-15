#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <algorithm>
#include <functional>

namespace Spark::Gui {

class Button;
class List;
class Panel;
class GuiPaintContext;

/** Collapsed row + expandable option list. */
class Dropdown final : public Widget {
public:
    Dropdown();
    void SetHeaderFontSize(float px) noexcept;
    void SetListItemFontSize(float px) noexcept;
    void SetRowHeight(float h) noexcept;
    void SetOptions(Array<Utf8String> opts);
    [[nodiscard]] int GetSelectedIndex() const noexcept { return selectedIndex; }
    /** Updates header and selection without firing SetOnSelectionChanged; keeps the popup closed. */
    void SetSelectedIndex(int i) noexcept;
    void SetOnSelectionChanged(std::function<void(int)> fn) { onSelect = Spark::MoveTemp(fn); }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    /** Late-layer popup body; call after the widget tree so it composites above siblings. */
    void PaintOpenPopup(GuiPaintContext& ctx) const;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;

    [[nodiscard]] bool IsPopupOpen() const noexcept { return open; }
    void ClosePopup() noexcept;
    /** Header row, or open list panel when <c>open</c>. */
    [[nodiscard]] bool HitPopupSurface(float x, float y) const noexcept;
    void SetMaxVisibleRows(int rows) noexcept { maxVisibleRows = std::max(1, rows); }
    /** When true, the option list opens above the header (avoids covering content below). */
    void SetOpenUpward(bool upward) noexcept { openUpward = upward; }

private:
    void RebuildHeader();
    void ToggleOpen();

    Array<Utf8String> options{};
    int selectedIndex = 0;
    bool open = false;
    float rowHeight = 28.0F;
    std::function<void(int)> onSelect{};

    Button* headerBtn = nullptr;
    Panel* dropPanel = nullptr;
    List* list = nullptr;
    float headerFontPx = 22.0F;
    float listItemFontPx = 22.0F;
    int maxVisibleRows = 6;
    bool openUpward = false;
};

}  // namespace Spark::Gui
