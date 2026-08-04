#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/factory/ControlDesc.hpp"

namespace Spark {

class IInput;
class UiCanvasComponent;

namespace Ui {

struct TreeViewItem {
    Utf8String label{};
    int parent = -1;
    bool expanded = true;
};

/**
 * Hierarchical list with expand/collapse and vertical scroll. Add items with <c>AddItem</c> (<c>parent == -1</c>
 * for roots). Row hit uses the same scrollbar strip as <c>SparkList</c>.
 */
class SparkTreeView final : public ITreeView, public UiElementBase {
public:
    struct RowBinding {
        SparkTreeView* tree = nullptr;
        int row = -1;
        int nodeId = -1;
        int depth = 0;
        bool hasKids = false;
    };

    explicit SparkTreeView(const TreeViewDesc& desc);

    void Clear() override;
    int AddItem(int parentIndex, Utf8String label) override;
    [[nodiscard]] int GetSelectedNodeId() const noexcept override { return selectedNodeId; }
    void SetOnSelectionChanged(UiIntCallback handler) override { onSelect = handler; }

    [[nodiscard]] bool WantsScrollInput() const override { return true; }
    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }
    void ProcessKeyInput(IInput& input) override;
    void OnScroll(float deltaX, float deltaY) override;

    void Arrange(const Rect& finalBounds) override;
    void Paint(IUiRenderer& renderer) override;
    [[nodiscard]] IUiElement* HitTest(float x, float y) override;
    void OnPointerDown(const UiFrameInput& input, UiCanvasComponent& canvas) override;
    void OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& canvas) override;
    void OnPointerUp(const UiFrameInput& input, UiCanvasComponent& canvas) override;

    void HandleRowClick(int row, int nodeId, int depth, bool hasKids, const UiFrameInput& input);

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;

private:
    struct VisibleRow {
        int nodeId = 0;
        int depth = 0;
    };

    [[nodiscard]] float ScaledRowHeight() const noexcept;
    [[nodiscard]] float ScaledItemFontPx() const noexcept;
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
    void SyncScrollLayout() noexcept;
    void ApplyScrollWheelDelta(float deltaY) noexcept;

    float rowHeight = 0.0F;
    float itemFontPx = 0.0F;
    Array<TreeViewItem> nodes{};
    Array<Array<int>> nodeChildren{};
    Array<VisibleRow> visibleRows{};
    Array<RowBinding> rowBindings{};

    int selectedNodeId = -1;
    UiIntCallback onSelect{};

    float scrollY = 0.0F;
    float contentHeight = 0.0F;
    float maxScroll = 0.0F;
    bool draggingThumb = false;
    float grabOffsetY = 0.0F;
    Rect trackRect{};
    Rect thumbRect{};
    Rect arrangeRect{};
    static constexpr float kTrackWidth = 12.0F;
};

}  // namespace Ui
}  // namespace Spark
