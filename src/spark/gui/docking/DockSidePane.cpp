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
    title_ = title;
    RebuildChrome();
}

void DockSidePane::SetCollapsed(const bool collapsed) noexcept {
    if (collapsed_ == collapsed) {
        return;
    }
    collapsed_ = collapsed;
    ApplyCollapsedVisuals();
    if (onCollapsedChanged_) {
        onCollapsedChanged_(collapsed_);
    }
}

void DockSidePane::ToggleCollapsed() {
    SetCollapsed(!collapsed_);
}

void DockSidePane::SetContentWidget(UniquePtr<Widget> content) {
    content_ = MoveTemp(content);
}

float DockSidePane::GetOccupiedWidth() const noexcept {
    if (collapsed_) {
        return collapsedStripPx_;
    }
    return paneWidthPx_ + gutterHalfPx_ * 2.0F;
}

void DockSidePane::Arrange(const Rect& r) {
    bounds = r;
    const float g2 = gutterHalfPx_ * 2.0F;
    const GuiLayoutMetrics& m = GetActiveGuiLayoutMetrics();
    const float headerH = m.Scaled(headerHeightPx_);

    if (collapsed_) {
        gutterRect_ = {0.0F, 0.0F, 0.0F, 0.0F};
        contentRect_ = {0.0F, 0.0F, 0.0F, 0.0F};
        headerRect_ = {0.0F, 0.0F, 0.0F, 0.0F};
        if (expandStrip_) {
            expandStrip_->SetVisible(true);
            expandStrip_->Arrange(r);
        }
        if (headerBar_) {
            headerBar_->SetVisible(false);
        }
        if (content_) {
            content_->SetVisible(false);
        }
        return;
    }

    if (expandStrip_) {
        expandStrip_->SetVisible(false);
    }
    if (headerBar_) {
        headerBar_->SetVisible(true);
    }
    if (content_) {
        content_->SetVisible(true);
    }

    const float maxPane = std::max(160.0F, r.width - g2);
    paneWidthPx_ = std::clamp(paneWidthPx_, 160.0F, maxPane);

    float paneX = r.x;
    if (edge_ == Edge::Right) {
        paneX = r.x + r.width - paneWidthPx_;
        gutterRect_ = {r.x, r.y, g2, r.height};
        contentRect_ = {paneX, r.y, paneWidthPx_, r.height};
    } else {
        gutterRect_ = {r.x + paneWidthPx_, r.y, g2, r.height};
        contentRect_ = {r.x, r.y, paneWidthPx_, r.height};
    }

    headerRect_ = {contentRect_.x, contentRect_.y, contentRect_.width, headerH};
    const Rect bodyRect{
            contentRect_.x,
            contentRect_.y + headerH,
            contentRect_.width,
            std::max(0.0F, contentRect_.height - headerH)};

    if (headerBar_) {
        headerBar_->Arrange(headerRect_);
    }
    if (collapseButton_) {
        const float btnW = 28.0F;
        collapseButton_->Arrange(
                {headerRect_.x + headerRect_.width - btnW - 4.0F, headerRect_.y + 2.0F, btnW, headerRect_.height - 4.0F});
    }
    if (content_) {
        content_->Arrange(bodyRect);
    }
}

void DockSidePane::Paint(GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    if (collapsed_) {
        if (expandStrip_) {
            expandStrip_->Paint(ctx);
        }
        return;
    }

    if (headerBar_) {
        headerBar_->Paint(ctx);
    }
    if (collapseButton_ && collapseButton_->IsVisible()) {
        collapseButton_->Paint(ctx);
    }
    if (content_) {
        content_->Paint(ctx);
    }

    if (gutterRect_.width > 0.0F) {
        const GuiTheme& th = ctx.GetTheme();
        ctx.PushOverlayLayer();
        ctx.FillRect(gutterRect_.x, gutterRect_.y, gutterRect_.width, gutterRect_.height, th.insetTrackRgb, 1.0F);
        ctx.StrokeRect(gutterRect_.x, gutterRect_.y, gutterRect_.width, gutterRect_.height, 1.0F, th.borderRgb, 0.65F);
        ctx.PopOverlayLayer();
    }
}

Widget* DockSidePane::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    if (!collapsed_ && HitGutter(x, y)) {
        return this;
    }
    if (collapsed_ && expandStrip_) {
        return expandStrip_->FindDeepestHover(x, y);
    }
    if (collapseButton_ && collapseButton_->IsVisible()) {
        if (Widget* hit = collapseButton_->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (headerBar_ && headerBar_->IsVisible()) {
        if (Widget* hit = headerBar_->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (content_ && content_->IsVisible()) {
        if (Widget* hit = content_->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    return nullptr;
}

void DockSidePane::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) {
    if (!collapsed_ && HitGutter(in.mouseX, in.mouseY)) {
        draggingGutter_ = true;
        return;
    }
    Widget::NotifyPointerDown(in, canvas);
}

void DockSidePane::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) {
    if (draggingGutter_) {
        const float g2 = gutterHalfPx_ * 2.0F;
        const float maxPane = std::max(160.0F, bounds.width - g2);
        if (edge_ == Edge::Left) {
            paneWidthPx_ = std::clamp(in.mouseX - bounds.x - gutterHalfPx_, 160.0F, maxPane);
        } else {
            paneWidthPx_ = std::clamp(bounds.x + bounds.width - in.mouseX - gutterHalfPx_, 160.0F, maxPane);
        }
        NotifyWidthChanged(false);
        return;
    }
    Widget::NotifyPointerDrag(in, canvas);
}

void DockSidePane::NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) {
    if (draggingGutter_) {
        NotifyWidthChanged(true);
        draggingGutter_ = false;
    }
    Widget::NotifyPointerUp(in, canvas);
}

void DockSidePane::RebuildChrome() {
    ClearChildren();
    headerBar_ = nullptr;
    collapseButton_ = nullptr;
    expandStrip_ = nullptr;

    auto header = MakeUnique<Panel>();
    StyleSideChrome(*header);
    header->SetPadding(4.0F);
    header->SetChromeEnabled(false);
    header->SetHitTest(false);

    auto titleLabel = MakeUnique<Label>();
    titleLabel->SetText(title_);
    titleLabel->SetFontSize(18.0F);
    titleLabel->SetTextColor(GuiTheme::SceneEditorDark().labelPrimary);
    titleLabel->SetHitTest(false);
    header->AddChild(MoveTemp(titleLabel));

    headerBar_ = header.Get();
    AddChild(MoveTemp(header));

    auto collapseBtn = MakeUnique<Button>();
    collapseBtn->SetFontSize(16.0F);
    collapseBtn->SetOpaqueSurface(true);
    collapseBtn->SetOnClick([this]() { ToggleCollapsed(); });
    collapseButton_ = collapseBtn.Get();
    AddChild(MoveTemp(collapseBtn));

    auto expand = MakeUnique<Button>();
    expand->SetOpaqueSurface(true);
    expand->SetFontSize(16.0F);
    expand->SetOnClick([this]() { ToggleCollapsed(); });
    expandStrip_ = expand.Get();
    AddChild(MoveTemp(expand));

    ApplyCollapsedVisuals();
}

void DockSidePane::ApplyCollapsedVisuals() {
    Utf8String expandLabel;
    if (edge_ == Edge::Left) {
        expandLabel.AppendUtf8("▶ ");
        expandLabel.AppendUtf8(title_);
    } else {
        expandLabel.AppendUtf8(title_);
        expandLabel.AppendUtf8(" ◀");
    }
    const Utf8String collapseLabel = edge_ == Edge::Left ? Utf8String("◀") : Utf8String("▶");
    if (collapseButton_) {
        if (Button* btn = dynamic_cast<Button*>(collapseButton_)) {
            btn->SetLabel(collapseLabel);
        }
    }
    if (expandStrip_) {
        if (Button* btn = dynamic_cast<Button*>(expandStrip_)) {
            btn->SetLabel(collapsed_ ? expandLabel : Utf8String(""));
        }
        expandStrip_->SetVisible(collapsed_);
    }
    if (headerBar_) {
        headerBar_->SetVisible(!collapsed_);
    }
    if (content_) {
        content_->SetVisible(!collapsed_);
    }
}

bool DockSidePane::HitGutter(const float x, const float y) const noexcept {
    return gutterRect_.width > 0.0F && gutterRect_.Contains(x, y);
}

void DockSidePane::NotifyWidthChanged(const bool committed) {
    if (onPaneWidthChanged_) {
        onPaneWidthChanged_(paneWidthPx_, committed);
    }
}

}  // namespace Spark::Gui
