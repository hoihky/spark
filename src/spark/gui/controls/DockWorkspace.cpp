#include "spark/gui/DockLayoutStore.hpp"
#include "spark/gui/controls/DockWorkspace.hpp"

#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/controls/Panel.hpp"
#include "spark/gui/controls/Splitter.hpp"
#include "spark/gui/controls/TabControl.hpp"

#include <algorithm>

namespace Spark::Gui {

namespace {

/** Arranges/paints a panel owned elsewhere; supports passthrough hit testing. */
class DockLeafHost final : public Widget {
public:
    void SetContent(Widget* widget) noexcept { content = widget; }
    void SetPassthrough(bool value) noexcept { passthrough = value; }
    void SetChromePanel(Panel* panel) noexcept { chrome = panel; }
  void SetPadding(float pad) noexcept { padding = pad; }

    void Arrange(const Rect& r) override {
        bounds = r;
        Rect inner = r;
        if (chrome != nullptr) {
            chrome->Arrange(r);
            inner = r.Inset(padding);
        }
        if (content != nullptr) {
            content->Arrange(inner);
        }
    }

    void Paint(GuiPaintContext& ctx) const override {
        if (chrome != nullptr) {
            chrome->Paint(ctx);
        }
        if (content != nullptr && content->IsVisible()) {
            content->Paint(ctx);
        }
    }

    Widget* FindDeepestHover(const float x, const float y) override {
        if (!visible || !enabled) {
            return nullptr;
        }
        if (content != nullptr) {
            if (Widget* hit = content->FindDeepestHover(x, y)) {
                return hit;
            }
        }
        if (passthrough) {
            return nullptr;
        }
        if (hitTest && bounds.Contains(x, y)) {
            return this;
        }
        return nullptr;
    }

private:
    Widget* content = nullptr;
    Panel* chrome = nullptr;
    bool passthrough = false;
    float padding = 4.0F;
};

/** Horizontal leading-pixel split with draggable gutter (sidebar). */
class DockLeadingSplit final : public Widget {
public:
    void SetLeadingSize(float px) noexcept { leadingPx = px; }
    [[nodiscard]] float GetLeadingSize() const noexcept { return leadingPx; }

    void SetOnLeadingSizeChanged(std::function<void(float px, bool committed)> fn) {
        onLeadingSizeChanged = Spark::MoveTemp(fn);
    }

    void Arrange(const Rect& r) override {
        bounds = r;
        const auto& ch = GetChildren();
        if (ch.GetSize() < 2U) {
            Widget::Arrange(r);
            gutterRect = {0.0F, 0.0F, 0.0F, 0.0F};
            return;
        }
        const float g2 = gutterHalf * 2.0F;
        const float maxLeading = std::max(160.0F, r.width * 0.5F - g2);
        leadingPx = std::clamp(leadingPx, 160.0F, maxLeading);
        if (ch[0]) {
            ch[0]->Arrange({r.x, r.y, leadingPx, r.height});
        }
        const float rightX = r.x + leadingPx + g2;
        if (ch[1]) {
            ch[1]->SetHitTest(false);
            ch[1]->Arrange({rightX, r.y, std::max(0.0F, r.width - leadingPx - g2), r.height});
        }
        gutterRect = {r.x + leadingPx, r.y, g2, r.height};
    }

    void Paint(GuiPaintContext& ctx) const override {
        PaintChildren(ctx);
        if (gutterRect.width <= 0.0F) {
            return;
        }
        const GuiTheme& th = ctx.GetTheme();
        ctx.PushOverlayLayer();
        ctx.FillRect(gutterRect.x, gutterRect.y, gutterRect.width, gutterRect.height, th.insetTrackRgb, 1.0F);
        ctx.StrokeRect(gutterRect.x, gutterRect.y, gutterRect.width, gutterRect.height, 1.0F, th.borderRgb, 0.65F);
        ctx.PopOverlayLayer();
    }

    Widget* FindDeepestHover(const float x, const float y) override {
        if (!visible || !enabled) {
            return nullptr;
        }
        if (HitGutter(x, y)) {
            return this;
        }
        const auto& ch = GetChildren();
        if (ch.GetSize() >= 2U && ch[1]) {
            if (Widget* hit = ch[1]->FindDeepestHover(x, y)) {
                return hit;
            }
        }
        if (ch.GetSize() >= 1U && ch[0]) {
            if (Widget* hit = ch[0]->FindDeepestHover(x, y)) {
                return hit;
            }
        }
        return nullptr;
    }

    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) override {
        if (HitGutter(in.mouseX, in.mouseY)) {
            dragging = true;
        }
    }

    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) override {
        if (!dragging) {
            return;
        }
        const float g2 = gutterHalf * 2.0F;
        const float maxLeading = std::max(160.0F, bounds.width * 0.5F - g2);
        leadingPx = std::clamp(in.mouseX - bounds.x - gutterHalf, 160.0F, maxLeading);
        if (onLeadingSizeChanged) {
            onLeadingSizeChanged(leadingPx, false);
        }
    }

    void NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) override {
        if (dragging && onLeadingSizeChanged) {
            onLeadingSizeChanged(leadingPx, true);
        }
        dragging = false;
    }

private:
    [[nodiscard]] bool HitGutter(const float x, const float y) const noexcept {
        return gutterRect.width > 0.0F && gutterRect.Contains(x, y);
    }

    float leadingPx = 300.0F;
    float gutterHalf = 3.0F;
    Rect gutterRect{};
    bool dragging = false;
    std::function<void(float px, bool committed)> onLeadingSizeChanged{};
};

void StyleElevatedChrome(Panel& panel) {
    const GuiTheme& th = GuiTheme::SceneEditorDark();
    panel.SetBackgroundGradient(th.panelElevatedTop, th.panelElevatedBottom, th.panelElevatedAlpha);
    panel.SetChromeEnabled(true);
    panel.SetDropShadowEnabled(false);
}

}  // namespace

void DockWorkspace::SetLayout(DockLayoutModel layoutModel) {
    model = MoveTemp(layoutModel);
    MarkLayoutDirty();
}

void DockWorkspace::SetPanel(const Utf8String panelId, UniquePtr<Widget> content, const bool useChrome) {
    for (std::size_t i = 0; i < panels.GetSize(); ++i) {
        if (panels[i].id == panelId) {
            panels[i].content = MoveTemp(content);
            panels[i].useChrome = useChrome;
            MarkLayoutDirty();
            return;
        }
    }
    PanelSlotEntry entry{};
    entry.id = panelId;
    entry.content = MoveTemp(content);
    entry.useChrome = useChrome;
    panels.PushBack(MoveTemp(entry));
    MarkLayoutDirty();
}

Widget* DockWorkspace::GetPanelWidget(const Utf8String& panelId) noexcept {
    return FindPanelContent(panelId);
}

void DockWorkspace::Arrange(const Rect& r) {
    bounds = r;
    RebuildIfNeeded();
    if (builtRoot) {
        builtRoot->Arrange(r);
    }
}

void DockWorkspace::Paint(GuiPaintContext& ctx) const {
    if (builtRoot) {
        builtRoot->Paint(ctx);
    }
}

Widget* DockWorkspace::FindDeepestHover(const float x, const float y) {
    if (!visible || !enabled || !builtRoot) {
        return nullptr;
    }
    return builtRoot->FindDeepestHover(x, y);
}

void DockWorkspace::RebuildIfNeeded() {
    if (!layoutDirty) {
        return;
    }
    builtRoot = BuildNode(model.GetRoot());
    layoutDirty = false;
}

UniquePtr<Widget> DockWorkspace::BuildNode(const int nodeIndex) {
    const DockNode* node = model.GetNode(nodeIndex);
    if (node == nullptr) {
        return {};
    }
    switch (node->kind) {
    case DockNodeKind::Leaf:
        return BuildLeaf(*node);
    case DockNodeKind::Tabs:
        return BuildTabs(*node, nodeIndex);
    case DockNodeKind::Split:
        return BuildSplit(*node, nodeIndex);
    }
    return {};
}

UniquePtr<Widget> DockWorkspace::BuildLeaf(const DockNode& node) {
    auto host = MakeUnique<DockLeafHost>();
    host->SetPassthrough(node.passthroughInput);

    Widget* content = FindPanelContent(node.panelId);
    if (content == nullptr && node.passthroughInput) {
        auto placeholder = MakeUnique<Panel>();
        placeholder->SetHitTest(false);
        placeholder->SetBackgroundEnabled(false);
        placeholder->SetChromeEnabled(false);
        SetPanel(node.panelId, UniquePtr<Widget>(static_cast<Widget*>(placeholder.Release())), false);
        content = FindPanelContent(node.panelId);
    }

    if (content != nullptr) {
        bool useChrome = true;
        for (std::size_t i = 0; i < panels.GetSize(); ++i) {
            if (panels[i].id == node.panelId) {
                useChrome = panels[i].useChrome;
                break;
            }
        }
        if (useChrome && !node.passthroughInput) {
            auto chrome = MakeUnique<Panel>();
            StyleElevatedChrome(*chrome);
            chrome->SetPadding(4.0F);
            Panel* chromeRaw = chrome.Get();
            host->SetChromePanel(chromeRaw);
            host->AddChild(MoveTemp(chrome));
        }
        host->SetContent(content);
    }
    return UniquePtr<Widget>(host.Release());
}

UniquePtr<Widget> DockWorkspace::BuildTabs(const DockNode& node, const int nodeIndex) {
    auto tabs = MakeUnique<TabControl>();
    tabs->SetTabBarHeight(36.0F);
    for (std::size_t i = 0; i < node.tabs.GetSize(); ++i) {
        const DockTabSpec& spec = node.tabs[i];
        UniquePtr<Widget> page;
        if (spec.contentNodeIndex >= 0) {
            page = BuildNode(spec.contentNodeIndex);
        } else {
            DockNode leaf{};
            leaf.kind = DockNodeKind::Leaf;
            leaf.panelId = spec.panelId;
            page = BuildLeaf(leaf);
        }
        if (page) {
            tabs->AddTabWidget(spec.title, MoveTemp(page));
        }
    }
    tabs->SetSelectedIndex(node.selectedTab);
    tabs->SetOnTabChanged([this, nodeIndex](const int tabIndex) {
        if (DockNode* mutableNode = model.GetNode(nodeIndex)) {
            mutableNode->selectedTab = tabIndex;
            NotifyLayoutChanged(true);
        }
    });
    return UniquePtr<Widget>(tabs.Release());
}

UniquePtr<Widget> DockWorkspace::BuildSplit(const DockNode& node, const int nodeIndex) {
    UniquePtr<Widget> first = BuildNode(node.firstChild);
    UniquePtr<Widget> second = BuildNode(node.secondChild);
    if (!first || !second) {
        if (first) {
            return first;
        }
        return second;
    }

    if (node.measure == DockSplitMeasure::LeadingPixels && node.axis == DockSplitAxis::Horizontal) {
        auto split = MakeUnique<DockLeadingSplit>();
        split->SetLeadingSize(node.splitValue);
        split->SetOnLeadingSizeChanged([this, nodeIndex](const float px, const bool committed) {
            if (DockNode* mutableNode = model.GetNode(nodeIndex)) {
                mutableNode->splitValue = px;
            }
            NotifyLayoutChanged(committed);
        });
        split->AddChild(MoveTemp(first));
        split->AddChild(MoveTemp(second));
        return UniquePtr<Widget>(split.Release());
    }

    auto splitter = MakeUnique<Splitter>();
    splitter->SetOrientation(node.axis == DockSplitAxis::Horizontal ? SplitterOrientation::Horizontal
                                                                    : SplitterOrientation::Vertical);
    splitter->SetSplit(node.splitValue);
    splitter->SetOnSplitChanged([this, nodeIndex](const float fraction, const bool committed) {
        if (DockNode* mutableNode = model.GetNode(nodeIndex)) {
            mutableNode->splitValue = fraction;
        }
        NotifyLayoutChanged(committed);
    });
    splitter->AddChild(MoveTemp(first));
    splitter->AddChild(MoveTemp(second));
    return UniquePtr<Widget>(splitter.Release());
}

Widget* DockWorkspace::FindPanelContent(const Utf8String& panelId) noexcept {
    for (std::size_t i = 0; i < panels.GetSize(); ++i) {
        if (panels[i].id == panelId) {
            return panels[i].content.Get();
        }
    }
    return nullptr;
}

void DockWorkspace::NotifyLayoutChanged(const bool committed) {
    if (onLayoutChanged) {
        onLayoutChanged(model);
    }
    if (committed) {
        (void)SaveDockLayout(model);
    }
}

}  // namespace Spark::Gui
