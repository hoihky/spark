#include "spark/gui/controls/Dropdown.hpp"

#include "spark/gui/controls/Button.hpp"
#include "spark/gui/controls/List.hpp"
#include "spark/gui/controls/Panel.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <algorithm>

namespace Spark::Gui {

namespace {

constexpr Rect kCollapsedPopupRect{0.0F, 0.0F, 0.0F, 0.0F};

}  // namespace

void Dropdown::ClosePopup() noexcept {
    open = false;
    if (dropPanel != nullptr) {
        dropPanel->SetVisible(false);
    }
    if (list != nullptr) {
        list->SetVisible(false);
    }
}

bool Dropdown::HitPopupSurface(const float x, const float y) const noexcept {
    if (headerBtn != nullptr && headerBtn->GetBounds().Contains(x, y)) {
        return true;
    }
    if (open && dropPanel != nullptr && dropPanel->IsVisible() && dropPanel->GetBounds().Contains(x, y)) {
        return true;
    }
    return false;
}

void Dropdown::SetHeaderFontSize(const float px) noexcept {
    headerFontPx = px;
    if (headerBtn != nullptr) {
        headerBtn->SetFontSize(px);
    }
}

void Dropdown::SetListItemFontSize(const float px) noexcept {
    listItemFontPx = px;
    if (list != nullptr) {
        list->SetItemFontSize(px);
    }
}

void Dropdown::SetRowHeight(const float h) noexcept {
    rowHeight = h;
    if (list != nullptr) {
        list->SetRowHeight(h);
    }
}

Dropdown::Dropdown() {
    auto h = MakeUnique<Button>();
    headerBtn = h.Get();
    headerBtn->SetLabel(Utf8String("Select..."));
    headerBtn->SetFontSize(headerFontPx);
    headerBtn->SetOnClick([this]() { ToggleOpen(); });
    AddChild(Spark::MoveTemp(h));

    auto dp = MakeUnique<Panel>();
    dropPanel = dp.Get();
    dropPanel->SetVisible(false);
    dropPanel->SetPadding(0.0F);
    dropPanel->SetDropdownListThemeBound(true);
    dropPanel->SetBackgroundEnabled(false);
    dropPanel->SetChromeEnabled(false);
    dropPanel->SetDropShadowEnabled(false);

    auto ls = MakeUnique<List>();
    list = ls.Get();
    list->SetRowHeight(rowHeight);
    list->SetItemFontSize(listItemFontPx);
    list->SetOpaqueRows(true);
    list->SetOpaqueViewport(true);
    list->SetOnSelectionChanged([this](const int i) {
        selectedIndex = i;
        ClosePopup();
        RebuildHeader();
        if (onSelect) {
            onSelect(i);
        }
    });
    dropPanel->AddChild(Spark::MoveTemp(ls));
    AddChild(Spark::MoveTemp(dp));
    ClosePopup();
}

void Dropdown::RebuildHeader() {
    if (headerBtn == nullptr) {
        return;
    }
    if (options.GetSize() == 0) {
        headerBtn->SetLabel(Utf8String("--"));
        return;
    }
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(options.GetSize())) {
        selectedIndex = 0;
    }
    headerBtn->SetLabel(options[static_cast<std::size_t>(selectedIndex)]);
}

void Dropdown::ToggleOpen() {
    open = !open;
    if (dropPanel != nullptr) {
        dropPanel->SetVisible(open);
    }
    if (list != nullptr) {
        list->SetVisible(open);
    }
}

void Dropdown::SetSelectedIndex(int i) noexcept {
    if (options.GetSize() == 0) {
        selectedIndex = 0;
        ClosePopup();
        RebuildHeader();
        return;
    }
    const int n = static_cast<int>(options.GetSize());
    selectedIndex = std::clamp(i, 0, n - 1);
    ClosePopup();
    RebuildHeader();
}

void Dropdown::SetOptions(Array<Utf8String> opts) {
    ClosePopup();
    options = Spark::MoveTemp(opts);
    if (options.GetSize() == 0) {
        selectedIndex = 0;
    } else if (selectedIndex >= static_cast<int>(options.GetSize())) {
        selectedIndex = static_cast<int>(options.GetSize()) - 1;
    }
    if (list != nullptr) {
        list->SetItems(options);
    }
    RebuildHeader();
}

void Dropdown::Arrange(const Rect& r) {
    const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
    const float rowH = m.Scaled(rowHeight);
    const float headerH = r.height > 0.5F ? r.height : rowH;
    /** Header slot only — popup is overflow and must not expand layout/hit bounds into siblings. */
    bounds = {r.x, r.y, r.width, headerH};
    if (headerBtn != nullptr) {
        headerBtn->Arrange({r.x, r.y, r.width, headerH});
    }
    if (dropPanel != nullptr && list != nullptr) {
        if (open) {
            dropPanel->SetVisible(true);
            list->SetVisible(true);
            const float fullListH = rowH * static_cast<float>(options.GetSize());
            const float maxListH = rowH * static_cast<float>(maxVisibleRows);
            const float listH = std::min(fullListH, maxListH);
            list->SetVerticalScrollingEnabled(fullListH > maxListH + 0.5F);
            float listY = r.y + headerH;
            if (openUpward) {
                listY = r.y - listH;
                if (listY < 0.0F) {
                    listY = 0.0F;
                }
            }
            dropPanel->Arrange({r.x, listY, r.width, listH});
            list->Arrange(dropPanel->GetBounds());
        } else {
            if (dropPanel != nullptr) {
                dropPanel->SetVisible(false);
                dropPanel->Arrange(kCollapsedPopupRect);
            }
            if (list != nullptr) {
                list->SetVisible(false);
                list->Arrange(kCollapsedPopupRect);
            }
        }
    } else if (dropPanel != nullptr) {
        ClosePopup();
    }
}

void Dropdown::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    if (headerBtn != nullptr) {
        headerBtn->Paint(ctx);
    }
}

void Dropdown::PaintOpenPopup(GuiPaintContext& ctx) const {
    if (!visible || !open || dropPanel == nullptr) {
        return;
    }
    const Rect& pb = dropPanel->GetBounds();
    if (pb.width <= 0.0F || pb.height <= 0.0F) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    ctx.PushLateLayer();
    ctx.FillRectGradientVertical(
            pb.x, pb.y, pb.width, pb.height, th.dropdownPanelTop, th.dropdownPanelBottom, 1.0F);
    dropPanel->Paint(ctx);
    ctx.PopLateLayer();
}

Widget* Dropdown::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (open && HitPopupSurface(x, y)) {
        if (dropPanel != nullptr && dropPanel->IsVisible()) {
            if (Widget* hit = dropPanel->FindDeepestHover(x, y)) {
                return hit;
            }
        }
        return this;
    }
    if (!open) {
        if (headerBtn != nullptr) {
            return headerBtn->FindDeepestHover(x, y);
        }
        return nullptr;
    }
    if (headerBtn != nullptr) {
        if (Widget* hit = headerBtn->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    return nullptr;
}

}  // namespace Spark::Gui
