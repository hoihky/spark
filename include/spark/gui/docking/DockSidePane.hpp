#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <functional>

namespace Spark {

class GuiCanvasComponent;

namespace Gui {

class GuiPaintContext;

/**
 * Collapsible, draggable dock rail attached to the left or right screen edge.
 * Expanded: header (title + collapse) + content. Collapsed: narrow expand strip.
 * Inner edge exposes a resize gutter.
 */
class DockSidePane final : public Widget {
public:
    enum class Edge {
        Left,
        Right,
    };

    DockSidePane();

    void SetEdge(Edge value) noexcept { edge = value; }
    void SetTitle(Utf8String title);
    void SetPaneWidth(float widthPx) noexcept { paneWidthPx = widthPx; }
    [[nodiscard]] float GetPaneWidth() const noexcept { return paneWidthPx; }

    void SetCollapsed(bool collapsed) noexcept;
    [[nodiscard]] bool IsCollapsed() const noexcept { return collapsed; }
    void ToggleCollapsed();

    void SetContent(UniquePtr<Widget> content) { SetContentWidget(MoveTemp(content)); }

    template<typename T>
        requires DerivedFrom<T, Widget>
    void SetContent(UniquePtr<T> content) {
        SetContentWidget(UniquePtr<Widget>(static_cast<Widget*>(content.Release())));
    }

    void SetOnPaneWidthChanged(std::function<void(float widthPx, bool committed)> fn) {
        onPaneWidthChanged = Spark::MoveTemp(fn);
    }
    void SetOnCollapsedChanged(std::function<void(bool collapsed)> fn) {
        onCollapsedChanged = Spark::MoveTemp(fn);
    }

    /** Width consumed in the parent layout (pane + gutter, or collapsed strip). */
    [[nodiscard]] float GetOccupiedWidth() const noexcept;

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;

private:
    void SetContentWidget(UniquePtr<Widget> content);
    void RebuildChrome();
    void ApplyCollapsedVisuals();
    [[nodiscard]] bool HitGutter(float x, float y) const noexcept;
    void NotifyWidthChanged(bool committed);

    Edge edge = Edge::Left;
    Utf8String title{Utf8String("Panel")};
    float paneWidthPx = 300.0F;
    float collapsedStripPx = 32.0F;
    float gutterHalfPx = 3.0F;
    float headerHeightPx = 30.0F;
    bool collapsed = false;
    bool draggingGutter = false;

    Rect gutterRect{};
    Rect contentRect{};
    Rect headerRect{};

    UniquePtr<Widget> content{};
    Widget* headerBar = nullptr;
    Widget* collapseButton = nullptr;
    Widget* expandStrip = nullptr;

    std::function<void(float widthPx, bool committed)> onPaneWidthChanged{};
    std::function<void(bool collapsed)> onCollapsedChanged{};
};

}  // namespace Gui
}  // namespace Spark
