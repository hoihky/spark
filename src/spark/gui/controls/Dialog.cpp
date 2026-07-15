#include "spark/gui/controls/Dialog.hpp"

#include "spark/gui/controls/Label.hpp"
#include "spark/gui/controls/Panel.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <algorithm>

namespace Spark::Gui {

namespace {

/** Lowest screen-space Y edge among <c>w</c> and visible descendants (for sizing the dialog body). */
float MaxDescendantBottomY(const Widget* w) {
    if (w == nullptr || !w->IsVisible()) {
        return -1.0e30F;
    }
    const Rect& b = w->GetBounds();
    float m = b.y + b.height;
    const Array<UniquePtr<Widget>>& kids = w->GetChildren();
    for (std::size_t i = 0; i < kids.GetSize(); ++i) {
        if (kids[i]) {
            m = std::max(m, MaxDescendantBottomY(kids[i].Get()));
        }
    }
    return m;
}

}  // namespace

Dialog::Dialog() {
    auto d = MakeUnique<Panel>();
    dimmer = d.Get();
    dimmer->SetBackgroundGradient({0.03F, 0.10F, 0.05F}, {0.06F, 0.14F, 0.08F}, 0.58F);
    dimmer->SetDropShadowEnabled(false);
    dimmer->SetChromeEnabled(false);
    AddChild(Spark::MoveTemp(d));

    auto t = MakeUnique<Label>();
    titleLabel = t.Get();
    titleLabel->SetFontSize(26.0F);
    titleLabel->SetText(Utf8String("Dialog"));
    titleLabel->SetTextColor({0.86F, 0.97F, 0.90F});
    AddChild(Spark::MoveTemp(t));

    auto b = MakeUnique<Panel>();
    body = b.Get();
    body->SetBackgroundGradient({0.84F, 0.95F, 0.80F}, {0.70F, 0.88F, 0.72F}, 0.94F);
    body->SetPadding(10.0F);
    AddChild(Spark::MoveTemp(b));
}

void Dialog::SetTitle(Utf8String t) {
    if (titleLabel != nullptr) {
        titleLabel->SetText(Spark::MoveTemp(t));
    }
}

void Dialog::SetTitleFontSize(const float px) {
    if (titleLabel != nullptr) {
        titleLabel->SetFontSize(px);
    }
}

void Dialog::Arrange(const Rect& r) {
    bounds = r;
    if (dimmer != nullptr) {
        dimmer->Arrange(r);
    }
    constexpr float titleH = 32.0F;
    constexpr float gap = 10.0F;
    const float stackH = titleH + gap + bodyH;
    const float topY = r.y + std::max(0.0F, (r.height - stackH) * 0.5F);
    const float cx = r.x + std::max(0.0F, (r.width - bodyW) * 0.5F);
    if (titleLabel != nullptr) {
        titleLabel->Arrange({cx, topY, bodyW, titleH});
    }
    if (body != nullptr) {
        const float bodyTop = topY + titleH + gap;
        body->Arrange({cx, bodyTop, bodyW, bodyH});
        /** Open dropdowns etc. may extend past the nominal body height; grow the body so panel chrome still
         *  covers the list blend destination (otherwise the dimmer shows through and the parent looks gone). */
        const float contentBottom = MaxDescendantBottomY(body);
        const float bodyBottom = bodyTop + bodyH;
        if (contentBottom > bodyBottom + 0.5F) {
            const float needH = contentBottom - bodyTop;
            body->Arrange({cx, bodyTop, bodyW, needH});
        }
    }
}

void Dialog::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const GuiTheme& th = ctx.GetTheme();
    if (dimmer != nullptr) {
        const_cast<Panel*>(dimmer)->SetBackgroundGradient(th.dialogDimmerTop, th.dialogDimmerBottom, th.dialogDimmerAlpha);
    }
    if (body != nullptr) {
        const_cast<Panel*>(body)->SetBackgroundGradient(th.panelElevatedTop, th.panelElevatedBottom, th.panelElevatedAlpha);
    }
    if (titleLabel != nullptr) {
        const_cast<Label*>(titleLabel)->SetTextColor(th.dialogTitleText);
    }
    Widget::Paint(ctx);
}

}  // namespace Spark::Gui
