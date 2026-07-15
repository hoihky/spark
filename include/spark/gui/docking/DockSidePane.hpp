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

    void SetEdge(Edge edge) noexcept { edge_ = edge; }
    void SetTitle(Utf8String title);
    void SetPaneWidth(float widthPx) noexcept { paneWidthPx_ = widthPx; }
    [[nodiscard]] float GetPaneWidth() const noexcept { return paneWidthPx_; }

    void SetCollapsed(bool collapsed) noexcept;
    [[nodiscard]] bool IsCollapsed() const noexcept { return collapsed_; }
    void ToggleCollapsed();

    void SetContent(UniquePtr<Widget> content) { SetContentWidget(MoveTemp(content)); }

    template<typename T>
        requires DerivedFrom<T, Widget>
    void SetContent(UniquePtr<T> content) {
        SetContentWidget(UniquePtr<Widget>(static_cast<Widget*>(content.Release())));
    }

    void SetOnPaneWidthChanged(std::function<void(float widthPx, bool committed)> fn) {
        onPaneWidthChanged_ = Spark::MoveTemp(fn);
    }
    void SetOnCollapsedChanged(std::function<void(bool collapsed)> fn) {
        onCollapsedChanged_ = Spark::MoveTemp(fn);
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

    Edge edge_ = Edge::Left;
    Utf8String title_{Utf8String("Panel")};
    float paneWidthPx_ = 300.0F;
    float collapsedStripPx_ = 32.0F;
    float gutterHalfPx_ = 3.0F;
    float headerHeightPx_ = 30.0F;
    bool collapsed_ = false;
    bool draggingGutter_ = false;

    Rect gutterRect_{};
    Rect contentRect_{};
    Rect headerRect_{};

    UniquePtr<Widget> content_{};
    Widget* headerBar_ = nullptr;
    Widget* collapseButton_ = nullptr;
    Widget* expandStrip_ = nullptr;

    std::function<void(float widthPx, bool committed)> onPaneWidthChanged_{};
    std::function<void(bool collapsed)> onCollapsedChanged_{};
};

}  // namespace Gui
}  // namespace Spark
