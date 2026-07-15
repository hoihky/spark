#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark {

class IInput;
class GuiCanvasComponent;

namespace Gui {

class GuiPaintContext;

struct TreeViewItem {
    Utf8String label{};
    int parent = -1;
    bool expanded = true;
};

/**
 * Hierarchical list with expand/collapse and vertical scroll. Add items with <c>AddItem</c> (<c>parent == -1</c>
 * for roots). Row hit uses the same scrollbar strip as <c>List</c>.
 */
class TreeView final : public Widget {
public:
    TreeView();
    void Clear();
    /** Returns new node index, or -1 if parent is invalid. */
    int AddItem(int parentIndex, Utf8String label);
    void SetRowHeight(float h) noexcept { rowHeight = h; }
    void SetItemFontSize(float px) noexcept { itemFontPx = px; }
    [[nodiscard]] int GetSelectedNodeId() const noexcept { return selectedNodeId; }
    void SetOnSelectionChanged(std::function<void(int nodeId)> fn) { onSelect = Spark::MoveTemp(fn); }

    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }
    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;
    void NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void NotifyPointerUp(const GuiFrameInput& in, GuiCanvasComponent& canvas) override;
    void ProcessKeyInput(IInput& input) override;

    void ApplyScrollWheelDelta(float deltaY) noexcept;

private:
    struct VisibleRow {
        int nodeId = 0;
        int depth = 0;
    };

    void RebuildChildrenMap();
    void RebuildVisibleRows();
    void RebuildButtons();
    void VisitNode(int id, int depth);
    [[nodiscard]] bool HitTrack(float x, float y) const noexcept;
    [[nodiscard]] bool RowIntersectsViewport(const Rect& rowBounds) const noexcept;
    void UpdateScrollFromThumbTop(float thumbTopY);
    void ToggleExpanded(int nodeId);
    [[nodiscard]] bool NodeHasChildren(int nodeId) const noexcept;
    void ScrollRowIntoView(int visibleRowIndex) noexcept;

    Array<TreeViewItem> nodes{};
    Array<Array<int>> nodeChildren{};
    Array<VisibleRow> visibleRows{};

    float rowHeight = 28.0F;
    float itemFontPx = 20.0F;
    int selectedNodeId = -1;
    std::function<void(int)> onSelect{};

    float scrollY = 0.0F;
    float contentHeight = 0.0F;
    float maxScroll = 0.0F;
    bool draggingThumb = false;
    float grabOffsetY = 0.0F;
    Rect trackRect{};
    Rect thumbRect{};
    static constexpr float kTrackW = 12.0F;
};

}  // namespace Gui

}  // namespace Spark
