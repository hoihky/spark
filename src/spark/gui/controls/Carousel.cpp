#include "spark/gui/controls/Carousel.hpp"

#include "spark/gui/controls/Button.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <algorithm>

namespace Spark::Gui {

Carousel::Carousel() = default;

void Carousel::SetItemFontSize(const float px) noexcept {
    itemFontPx = px;
    if (prevBtn != nullptr) {
        prevBtn->SetFontSize(std::max(13.0F, itemFontPx * 0.55F));
    }
    if (mainBtn != nullptr) {
        mainBtn->SetFontSize(itemFontPx);
    }
    if (nextBtn != nullptr) {
        nextBtn->SetFontSize(std::max(13.0F, itemFontPx * 0.55F));
    }
}

void Carousel::SetItemBold(const bool b) noexcept {
    itemBold = b;
    if (prevBtn != nullptr) {
        prevBtn->SetLabelBold(itemBold);
    }
    if (mainBtn != nullptr) {
        mainBtn->SetLabelBold(itemBold);
    }
    if (nextBtn != nullptr) {
        nextBtn->SetLabelBold(itemBold);
    }
}

void Carousel::SetOpaqueSlides(const bool v) noexcept {
    opaqueSlides = v;
    if (prevBtn != nullptr) {
        prevBtn->SetOpaqueSurface(opaqueSlides);
    }
    if (mainBtn != nullptr) {
        mainBtn->SetOpaqueSurface(opaqueSlides);
    }
    if (nextBtn != nullptr) {
        nextBtn->SetOpaqueSurface(opaqueSlides);
    }
}

void Carousel::RefreshMainLabel() noexcept {
    if (mainBtn == nullptr || items.IsEmpty()) {
        return;
    }
    const int n = static_cast<int>(items.GetSize());
    const int idx = std::clamp(selectedIndex, 0, n - 1);
    selectedIndex = idx;
    mainBtn->SetLabel(items[static_cast<std::size_t>(idx)]);
}

void Carousel::SyncAccent() noexcept {
    if (prevBtn != nullptr) {
        prevBtn->SetAccentSelected(false);
    }
    if (mainBtn != nullptr) {
        mainBtn->SetAccentSelected(true);
    }
    if (nextBtn != nullptr) {
        nextBtn->SetAccentSelected(false);
    }
}

void Carousel::Step(const int delta) noexcept {
    if (items.IsEmpty()) {
        return;
    }
    const int n = static_cast<int>(items.GetSize());
    selectedIndex = ((selectedIndex + delta) % n + n) % n;
    RefreshMainLabel();
    SyncAccent();
}

void Carousel::Rebuild() {
    children.Clear();
    prevBtn = nullptr;
    mainBtn = nullptr;
    nextBtn = nullptr;
    if (items.IsEmpty()) {
        selectedIndex = 0;
        return;
    }
    const int n = static_cast<int>(items.GetSize());
    selectedIndex = std::clamp(selectedIndex, 0, n - 1);

    {
        auto b = MakeUnique<Button>();
        prevBtn = b.Get();
        prevBtn->SetLabel(Utf8String("\xe2\x80\xb9"));
        prevBtn->SetFontSize(std::max(13.0F, itemFontPx * 0.55F));
        prevBtn->SetLabelBold(itemBold);
        prevBtn->SetOpaqueSurface(opaqueSlides);
        prevBtn->SetOnClick([this]() { Step(-1); });
        AddChild(Spark::MoveTemp(b));
    }
    {
        auto b = MakeUnique<Button>();
        mainBtn = b.Get();
        mainBtn->SetFontSize(itemFontPx);
        mainBtn->SetLabelBold(itemBold);
        mainBtn->SetOpaqueSurface(opaqueSlides);
        mainBtn->SetOnClick([this]() {
            if (onSelect && !items.IsEmpty()) {
                onSelect(selectedIndex);
            }
        });
        AddChild(Spark::MoveTemp(b));
    }
    {
        auto b = MakeUnique<Button>();
        nextBtn = b.Get();
        nextBtn->SetLabel(Utf8String("\xe2\x80\xba"));
        nextBtn->SetFontSize(std::max(13.0F, itemFontPx * 0.55F));
        nextBtn->SetLabelBold(itemBold);
        nextBtn->SetOpaqueSurface(opaqueSlides);
        nextBtn->SetOnClick([this]() { Step(1); });
        AddChild(Spark::MoveTemp(b));
    }
    RefreshMainLabel();
    SyncAccent();
}

void Carousel::SetItems(Array<Utf8String> lines) {
    items = Spark::MoveTemp(lines);
    if (items.IsEmpty()) {
        selectedIndex = 0;
        Rebuild();
        return;
    }
    selectedIndex = std::clamp(selectedIndex, 0, static_cast<int>(items.GetSize()) - 1);
    Rebuild();
}

void Carousel::SetSelectedIndex(const int i) noexcept {
    if (items.IsEmpty()) {
        selectedIndex = 0;
        return;
    }
    const int n = static_cast<int>(items.GetSize());
    selectedIndex = std::clamp(i, 0, n - 1);
    RefreshMainLabel();
    SyncAccent();
}

void Carousel::Arrange(const Rect& r) {
    bounds = r;
    if (children.GetSize() < 3 || prevBtn == nullptr || mainBtn == nullptr || nextBtn == nullptr) {
        return;
    }
    const float side = std::min(sideButtonWidth, std::max(36.0F, r.width * 0.075F));
    const float midW = std::max(96.0F, r.width - 2.0F * side - 2.0F * gap);
    const float h = r.height;
    prevBtn->Arrange({r.x, r.y, side, h});
    mainBtn->Arrange({r.x + side + gap, r.y, midW, h});
    nextBtn->Arrange({r.x + side + gap + midW + gap, r.y, side, h});
}

}  // namespace Spark::Gui
