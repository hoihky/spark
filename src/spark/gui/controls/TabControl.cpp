#include "spark/gui/controls/TabControl.hpp"

#include "spark/gui/controls/Button.hpp"
#include "spark/gui/controls/Panel.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <algorithm>

namespace Spark::Gui {

namespace {

/** Body shell for tab pages — only stores bounds; TabControl arranges pages itself. */
class TabPageHost final : public Widget {
public:
    TabPageHost() { SetHitTest(false); }

    void Arrange(const Rect& r) override { bounds = r; }

    void Paint(GuiPaintContext& ctx) const override {
        if (!visible) {
            return;
        }
        const GuiTheme& th = ctx.GetTheme();
        ctx.FillRectGradientVertical(
                bounds.x, bounds.y, bounds.width, bounds.height, th.tabBodyTop, th.tabBodyBottom, th.tabBodyAlpha);
        PaintChildren(ctx);
    }
};

}  // namespace

TabControl::TabControl() {
    auto hp = MakeUnique<Panel>();
    header = hp.Get();
    header->SetPadding(0.0F);
    header->SetChromeEnabled(false);
    header->SetDropShadowEnabled(false);
    header->SetBackgroundGradient({0.14F, 0.22F, 0.16F}, {0.08F, 0.14F, 0.10F}, 0.92F);
    AddChild(Spark::MoveTemp(hp));

    auto bp = MakeUnique<TabPageHost>();
    body = bp.Get();
    AddChild(Spark::MoveTemp(bp));
}

void TabControl::AddTabImpl(Utf8String title, UniquePtr<Widget> page) {
    const int idx = static_cast<int>(titles.GetSize());
    titles.PushBack(Spark::MoveTemp(title));
    page->SetVisible(idx == selected);
    body->AddChild(Spark::MoveTemp(page));

    auto btn = MakeUnique<Button>();
    btn->SetLabel(titles[static_cast<std::size_t>(idx)]);
    btn->SetFontSize(GetActiveGuiLayoutMetrics().fontControl);
    btn->SetLabelBold(true);
    btn->SetOpaqueSurface(true);
    btn->SetAccentSelected(idx == selected);
    btn->SetOnClick([this, idx]() { Select(idx); });
    header->AddChild(Spark::MoveTemp(btn));
}

void TabControl::Select(const int i) {
    if (i < 0 || i >= static_cast<int>(titles.GetSize())) {
        return;
    }
    selected = i;
    const auto& pages = body->GetChildren();
    for (std::size_t k = 0; k < pages.GetSize(); ++k) {
        if (pages[k]) {
            pages[k]->SetVisible(static_cast<int>(k) == i);
        }
    }
    const auto& tabs = header->GetChildren();
    for (std::size_t k = 0; k < tabs.GetSize(); ++k) {
        if (Button* b = dynamic_cast<Button*>(tabs[k].Get())) {
            b->SetAccentSelected(static_cast<int>(k) == i);
        }
    }
    if (onTabChanged) {
        onTabChanged(i);
    }
}

void TabControl::SetSelectedIndex(const int i) {
    Select(i);
}

void TabControl::Arrange(const Rect& r) {
    bounds = r;
    const float headerH = std::min(tabBarH, std::max(0.0F, r.height));
    if (header != nullptr) {
        header->Arrange({r.x, r.y, r.width, headerH});
    }
    const float bodyY = r.y + headerH;
    const float bodyH = std::max(0.0F, r.height - headerH);
    if (body != nullptr) {
        body->Arrange({r.x, bodyY, r.width, bodyH});
        constexpr float kPagePad = 4.0F;
        const Rect pageRect = body->GetBounds().Inset(kPagePad);
        const auto& pages = body->GetChildren();
        for (std::size_t k = 0; k < pages.GetSize(); ++k) {
            if (!pages[k]) {
                continue;
            }
            const bool show = static_cast<int>(k) == selected;
            pages[k]->SetVisible(show);
            if (show && pageRect.width > 0.0F && pageRect.height > 0.0F) {
                pages[k]->Arrange(pageRect);
            } else {
                pages[k]->Arrange({pageRect.x, pageRect.y, pageRect.width, 0.0F});
            }
        }
    }
    if (header == nullptr) {
        return;
    }
    const Rect hb = header->GetBounds();
    const auto& tabs = header->GetChildren();
    const std::size_t n = tabs.GetSize();
    if (n == 0) {
        return;
    }
    constexpr float innerPad = 6.0F;
    constexpr float gap = 6.0F;
    const float tabFont = GetActiveGuiLayoutMetrics().FontControl();
    const float minTabW = tabFont * 3.5F;
    const float totalGap = gap * static_cast<float>(n > 1U ? n - 1U : 0U);
    const float avail = std::max(minTabW * static_cast<float>(n), hb.width - 2.0F * innerPad - totalGap);
    const float bw = std::max(minTabW, avail / static_cast<float>(n));
    float x = hb.x + innerPad;
    const float btnH = std::max(tabFont + 8.0F, hb.height - 6.0F);
    const float btnY = hb.y + (hb.height - btnH) * 0.5F;
    for (std::size_t k = 0; k < n; ++k) {
        if (tabs[k]) {
            tabs[k]->Arrange({x, btnY, bw, btnH});
        }
        x += bw + gap;
    }
}

void TabControl::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    if (header != nullptr) {
        const_cast<Panel*>(header)->SetBackgroundGradient(th.tabHeaderTop, th.tabHeaderBottom, th.tabHeaderAlpha);
    }
    /** Header first, then body, so page content is never covered by the tab strip. */
    if (header != nullptr) {
        header->Paint(ctx);
    }
    if (body != nullptr) {
        body->Paint(ctx);
    }
}

}  // namespace Spark::Gui
