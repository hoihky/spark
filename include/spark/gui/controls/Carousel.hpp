#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark::Gui {

class Button;

/**
 * Horizontal carousel: ‹ / current item / ›. Side buttons step the focus; the center tile confirms and
 * invokes <c>SetOnSelectionChanged</c> with the current index.
 */
class Carousel final : public Widget {
public:
    Carousel();
    void SetSideButtonWidth(float w) noexcept { sideButtonWidth = w; }
    void SetGap(float g) noexcept { gap = g; }
    void SetItemFontSize(float px) noexcept;
    void SetItemBold(bool b) noexcept;
    void SetOpaqueSlides(bool v) noexcept;
    void SetItems(Array<Utf8String> lines);
    [[nodiscard]] int GetSelectedIndex() const noexcept { return selectedIndex; }
    /** Updates the focused item without invoking <c>SetOnSelectionChanged</c>. */
    void SetSelectedIndex(int i) noexcept;
    void SetOnSelectionChanged(std::function<void(int)> fn) { onSelect = Spark::MoveTemp(fn); }

    void Arrange(const Rect& r) override;

private:
    void Rebuild();
    void RefreshMainLabel() noexcept;
    void Step(int delta) noexcept;
    void SyncAccent() noexcept;

    float sideButtonWidth = 64.0F;
    float gap = 8.0F;
    float itemFontPx = 21.0F;
    bool itemBold = false;
    bool opaqueSlides = false;
    Array<Utf8String> items{};
    int selectedIndex = 0;
    Button* prevBtn = nullptr;
    Button* mainBtn = nullptr;
    Button* nextBtn = nullptr;
    std::function<void(int)> onSelect{};
};

}  // namespace Spark::Gui
