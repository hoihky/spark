#include "spark/demo/ShellDemoUi.hpp"

#include "spark/gui/controls/Button.hpp"
#include "spark/gui/controls/Label.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/GuiThemeCatalog.hpp"

namespace Spark {

namespace {

Spark::Gui::Label* AsLabel(Spark::Gui::Widget* w) noexcept {
    return dynamic_cast<Spark::Gui::Label*>(w);
}

}  // namespace

LauncherThemeRow::LauncherThemeRow() {
    auto caption = Spark::MakeUnique<Spark::Gui::Label>();
    caption->SetText(Spark::Utf8String("Theme"));
    caption->SetFontSize(20.0F);
    AddChild(Spark::MoveTemp(caption));

    auto prev = Spark::MakeUnique<Spark::Gui::Button>();
    prev->SetLabel(Spark::Utf8String("<"));
    prev->SetFontSize(18.0F);
    prev->SetOnClick([this]() {
        if (onThemeCycle) {
            onThemeCycle(-1);
        }
    });
    AddChild(Spark::MoveTemp(prev));

    auto name = Spark::MakeUnique<Spark::Gui::Label>();
    name->SetFontSize(18.0F);
    AddChild(Spark::MoveTemp(name));

    auto next = Spark::MakeUnique<Spark::Gui::Button>();
    next->SetLabel(Spark::Utf8String(">"));
    next->SetFontSize(18.0F);
    next->SetOnClick([this]() {
        if (onThemeCycle) {
            onThemeCycle(1);
        }
    });
    AddChild(Spark::MoveTemp(next));

    RefreshThemeName();
}

void LauncherThemeRow::RefreshThemeName() {
    if (children.GetSize() < 3U) {
        return;
    }
    const Spark::Gui::GuiThemePreset preset = Spark::Gui::GetActiveGuiThemePreset();
    if (children[2]) {
        if (Spark::Gui::Label* nameLbl = AsLabel(children[2].Get())) {
            nameLbl->SetText(Spark::Utf8String(Spark::Gui::GetGuiThemePresetDisplayName(preset)));
        }
    }
}

void LauncherThemeRow::Arrange(const Spark::Gui::Rect& r) {
    bounds = r;
    if (children.GetSize() < 4U || !children[0] || !children[1] || !children[2] || !children[3]) {
        return;
    }
    const Spark::Gui::GuiLayoutMetrics& m = Spark::Gui::GetActiveGuiLayoutMetrics();
    const float innerPad = m.Scaled(8.0F);
    const float gap = m.Scaled(8.0F);
    const float captionW = m.Scaled(72.0F);
    const float stepBtnW = m.Scaled(36.0F);
    const float innerH = std::max(0.0F, r.height - innerPad * 2.0F);
    const float y = r.y + innerPad;
    const float x0 = r.x + innerPad;

    children[0]->Arrange({x0, y, captionW, innerH});
    float x = x0 + captionW + gap;
    children[1]->Arrange({x, y, stepBtnW, innerH});
    x += stepBtnW + gap;
    const float nameW = std::max(m.Scaled(120.0F), r.width - innerPad * 2.0F - captionW - stepBtnW * 2.0F - gap * 3.0F);
    children[2]->Arrange({x, y, nameW, innerH});
    x += nameW + gap;
    children[3]->Arrange({x, y, stepBtnW, innerH});
}

void LauncherThemeRow::Paint(Spark::Gui::GuiPaintContext& ctx) const {
    if (!visible) {
        return;
    }
    const Spark::Gui::GuiTheme& th = ctx.GetTheme();
    const Spark::Gui::GuiLayoutMetrics& m = ctx.GetLayoutMetrics();
    ctx.FillRoundRectGradientVertical(bounds.x, bounds.y, bounds.width, bounds.height, m.Scaled(th.controlCornerRadius),
            th.panelElevatedTop, th.panelElevatedBottom, th.panelElevatedAlpha);
    ctx.StrokeRoundRect(bounds.x, bounds.y, bounds.width, bounds.height, m.Scaled(th.controlCornerRadius), 1.0F,
            th.borderRgb, th.controlStrokeAlpha);
    PaintChildren(ctx);
}

LauncherMenuLayout::LauncherMenuLayout(const float minListHeight) noexcept : minListH(minListHeight) {}

void LauncherMenuLayout::Arrange(const Spark::Gui::Rect& r) {
    bounds = r;
    const Spark::Gui::GuiLayoutMetrics& m = Spark::Gui::GetActiveGuiLayoutMetrics();
    constexpr float sidePadDesign = 48.0F;
    constexpr float topPadDesign = 20.0F;
    const float sidePad = m.Scaled(sidePadDesign);
    const float topPad = m.Scaled(topPadDesign);
    const float bottomPad = sidePad;
    const float innerW = (std::max)(m.Scaled(240.0F), r.width - sidePad * 2.0F);
    const float x = r.x + (r.width - innerW) * 0.5F;
    float y = r.y + topPad;

    if (children.GetSize() >= 2U && children[0] && children[1]) {
        const float themeH = m.Scaled(LauncherThemeRow::DesignHeight());
        children[0]->Arrange({x, y, innerW, themeH});
        y += themeH;
        const float listH = std::max(0.0F, r.height - y - bottomPad);
        children[1]->Arrange({x, y, innerW, listH});
        return;
    }
    if (children.GetSize() == 1U && children[0]) {
        const float innerH = std::max(0.0F, r.height - topPad - bottomPad);
        children[0]->Arrange({x, r.y + topPad, innerW, innerH});
    }
}

void LauncherMenuLayout::Paint(Spark::Gui::GuiPaintContext& ctx) const {
    PaintChildren(ctx);
}

Spark::Gui::Widget* LauncherMenuLayout::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled) {
        return nullptr;
    }
    /** Theme bar is painted on top — hit-test it before the demo list. */
    if (children.GetSize() >= 1U && children[0]) {
        if (Spark::Gui::Widget* hit = children[0]->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (children.GetSize() >= 2U && children[1]) {
        if (Spark::Gui::Widget* hit = children[1]->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (hitTest && bounds.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

GuiStackPanel::GuiStackPanel() = default;

GuiStackPanel::GuiStackPanel(const float rowH, const float gap) : rowHeight(rowH), spacing(gap) {}

void GuiStackPanel::Arrange(const Spark::Gui::Rect& r) {
    bounds = r;
    float y = r.y;
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i]) {
            children[i]->Arrange({r.x, y, r.width, rowHeight});
            y += rowHeight + spacing;
        }
    }
}

void GuiStackPanel::Paint(Spark::Gui::GuiPaintContext& ctx) const {
    PaintChildren(ctx);
}


}  // namespace Spark
