#include "spark/gui/docking/DockSidePane.hpp"

#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/controls/Button.hpp"
#include "spark/gui/controls/Label.hpp"
#include "spark/gui/controls/Panel.hpp"

#include <algorithm>

namespace Spark::Gui {

namespace {

void StyleSideChrome(Panel& panel) {
    const GuiTheme& th = GuiTheme::SceneEditorDark();
    panel.SetBackgroundGradient(th.panelElevatedTop, th.panelElevatedBottom, th.panelElevatedAlpha);
    panel.SetChromeEnabled(true);
    panel.SetDropShadowEnabled(false);
}

}  // namespace

DockSidePane::DockSidePane() {
    RebuildChrome();
}

void DockSidePane::SetTitle(const Utf8String title) {
    this->title = title;
    RebuildChrome();
}

void DockSidePane::SetCollapsed(const bool collapsed) noexcept {
    if (this->collapsed == collapsed) {
        return;
    }
    this->collapsed = collapsed;
    ApplyCollapsedVisuals();
    if (onCollapsedChanged) {
        onCollapsedChanged(collapsed);
    }
}

void DockSidePane::ToggleCollapsed() {
    SetCollapsed(!collapsed);
}

void DockSidePane::SetContentWidget(UniquePtr<Widget> content) {
    content = MoveTemp(content);
}

float DockSidePane::GetOccupiedWidth() const noexcept {
    if (collapsed) {
        return collapsedStripPx;
    }
    return paneWidthPx + gutterHalfPx * 2.0F;
}

void DockSidePane::Arrange(const Rect& r) {
    bounds = r;
    const float g2 = gutterHalfPx * 2.0F;
    const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
    const float headerH = m.Scaled(headerHeightPx);

    if (collapsed) {
        gutterRect = {0.0F, 0.0F, 0.0F, 0.0F};
        contentRect = {0.0F, 0.0F, 0.0F, 0.0F};
        headerRect = {0.0F, 0.0F, 0.0F, 0.0F};
        if (expandStrip) {
            expandStrip->SetVisible(true);
            expandStrip->Arrange(r);
        }
        if (headerBar) {
            headerBar->SetVisible(false);
        }
        if (content) {
            content->SetVisible(false);
        }
        return;
    }

    if (expandStrip) {
        expandStrip->SetVisible(false);
    }
    if (headerBar) {
        headerBar->SetVisible(true);
    }
    if (content) {
        content->SetVisible(true);
    }

    const float maxPane = std::max(160.0F, r.width - g2);
    paneWidthPx = std::clamp(paneWidthPx, 160.0F, maxPane);

    float paneX = r.x;
    if (edge == Edge::Right) {
        paneX = r.x + r.width - paneWidthPx;
        gutterRect = {r.x, r.y, g2, r.height};
        contentRect = {paneX, r.y, paneWidthPx, r.height};
    } else {
        gutterRect = {r.x + paneWidthPx, r.y, g2, r.height};
        contentRect = {r.x, r.y, paneWidthPx, r.height};
    }

    headerRect = {contentRect.x, contentRect.y, contentRect.width, headerH};
    const Rect bodyRect{
            contentRect.x,
            contentRect.y + headerH,
            contentRect.width,
            std::max(0.0F, contentRect.height - headerH)};

    if (headerBar) {
        headerBar->Arrange(headerRect);
    }
    if (collapseButton) {
        const float btnW = 28.0F;
        collapseButton->Arrange(
                {headerRect.x + headerRect.width - btnW - 4.0F, headerRect.y + 2.0F, btnW, headerRect.height - 4.0F});
    }
    if (content) {
        content->Arrange(bodyRect);
    }
}

void DockSidePane::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    if (collapsed) {
        if (expandStrip) {
            expandStrip->Paint(ctx);
        }
        return;
    }

    if (headerBar) {
        headerBar->Paint(ctx);
    }
    if (collapseButton && collapseButton->IsVisible()) {
        collapseButton->Paint(ctx);
    }
    if (content) {
        content->Paint(ctx);
    }

    if (gutterRect.width > 0.0F) {
        const GuiTheme& th = ctx.GetTheme();
        ctx.PushOverlayLayer();
        ctx.FillRect(gutterRect.x, gutterRect.y, gutterRect.width, gutterRect.height, th.insetTrackRgb, 1.0F);
        ctx.StrokeRect(gutterRect.x, gutterRect.y, gutterRect.width, gutterRect.height, 1.0F, th.borderRgb, 0.65F);
        ctx.PopOverlayLayer();
    }
}

Widget* DockSidePane::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (!collapsed && HitGutter(x, y)) {
        return this;
    }
    if (collapsed && expandStrip) {
        return expandStrip->FindDeepestHover(x, y);
    }
    if (collapseButton && collapseButton->IsVisible()) {
        if (Widget* hit = collapseButton->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (headerBar && headerBar->IsVisible()) {
        if (Widget* hit = headerBar->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (content && content->IsVisible()) {
        if (Widget* hit = content->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    return nullptr;
}

void DockSidePane::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) {
    if (!collapsed && HitGutter(in.mouseX, in.mouseY)) {
        draggingGutter = true;
        return;
    }
    Widget::NotifyPointerDown(in, canvas);
}

void DockSidePane::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) {
    if (draggingGutter) {
        const float g2 = gutterHalfPx * 2.0F;
        const float maxPane = std::max(160.0F, bounds.width - g2);
        if (edge == Edge::Left) {
            paneWidthPx = std::clamp(in.mouseX - bounds.x - gutterHalfPx, 160.0F, maxPane);
        } else {
            paneWidthPx = std::clamp(bounds.x + bounds.width - in.mouseX - gutterHalfPx, 160.0F, maxPane);
        }
        NotifyWidthChanged(false);
        return;
    }
    Widget::NotifyPointerDrag(in, canvas);
}

void DockSidePane::NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) {
    if (draggingGutter) {
        NotifyWidthChanged(true);
        draggingGutter = false;
    }
    Widget::NotifyPointerUp(in, canvas);
}

void DockSidePane::RebuildChrome() {
    ClearChildren();
    headerBar = nullptr;
    collapseButton = nullptr;
    expandStrip = nullptr;

    auto header = MakeUnique<Panel>();
    StyleSideChrome(*header);
    header->SetPadding(4.0F);
    header->SetChromeEnabled(false);
    header->SetHitTest(false);

    auto titleLabel = MakeUnique<Label>();
    titleLabel->SetText(title);
    titleLabel->SetFontSize(18.0F);
    titleLabel->SetTextColor(GuiTheme::SceneEditorDark().labelPrimary);
    titleLabel->SetHitTest(false);
    header->AddChild(MoveTemp(titleLabel));

    headerBar = header.Get();
    AddChild(MoveTemp(header));

    auto collapseBtn = MakeUnique<Button>();
    collapseBtn->SetFontSize(16.0F);
    collapseBtn->SetOpaqueSurface(true);
    collapseBtn->SetOnClick([this]() { ToggleCollapsed(); });
    collapseButton = collapseBtn.Get();
    AddChild(MoveTemp(collapseBtn));

    auto expand = MakeUnique<Button>();
    expand->SetOpaqueSurface(true);
    expand->SetFontSize(16.0F);
    expand->SetOnClick([this]() { ToggleCollapsed(); });
    expandStrip = expand.Get();
    AddChild(MoveTemp(expand));

    ApplyCollapsedVisuals();
}

void DockSidePane::ApplyCollapsedVisuals() {
    Utf8String expandLabel;
    if (edge == Edge::Left) {
        expandLabel.AppendUtf8("▶ ");
        expandLabel.AppendUtf8(title);
    } else {
        expandLabel.AppendUtf8(title);
        expandLabel.AppendUtf8(" ◀");
    }
    const Utf8String collapseLabel = edge == Edge::Left ? Utf8String("◀") : Utf8String("▶");
    if (collapseButton) {
        if (Button* btn = dynamic_cast<Button*>(collapseButton)) {
            btn->SetLabel(collapseLabel);
        }
    }
    if (expandStrip) {
        if (Button* btn = dynamic_cast<Button*>(expandStrip)) {
            btn->SetLabel(collapsed ? expandLabel : Utf8String(""));
        }
        expandStrip->SetVisible(collapsed);
    }
    if (headerBar) {
        headerBar->SetVisible(!collapsed);
    }
    if (content) {
        content->SetVisible(!collapsed);
    }
}

bool DockSidePane::HitGutter(const float x, const float y) const noexcept {
    return gutterRect.width > 0.0F && gutterRect.Contains(x, y);
}

void DockSidePane::NotifyWidthChanged(const bool committed) {
    if (onPaneWidthChanged) {
        onPaneWidthChanged(paneWidthPx, committed);
    }
}

}  // namespace Spark::Gui
