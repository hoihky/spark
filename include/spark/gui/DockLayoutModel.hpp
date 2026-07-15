#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

namespace Spark::Gui {

enum class DockNodeKind {
    Split,
    Tabs,
    Leaf,
};

enum class DockSplitAxis {
    Horizontal,
    Vertical,
};

/** How <c>splitValue</c> on a split node is interpreted. */
enum class DockSplitMeasure {
    /** First pane fraction along the inner axis (0.08–0.92). */
    Fraction,
    /** Leading pane size in pixels (horizontal = width, vertical = height). */
    LeadingPixels,
};

struct DockTabSpec {
    Utf8String title;
    /** Panel id when the tab hosts a leaf panel directly. */
    Utf8String panelId;
    /** Nested layout node when <c>contentNodeIndex >= 0</c>. */
    int contentNodeIndex = -1;
};

struct DockNode {
    DockNodeKind kind = DockNodeKind::Leaf;

    DockSplitAxis axis = DockSplitAxis::Horizontal;
    DockSplitMeasure measure = DockSplitMeasure::Fraction;
    float splitValue = 0.5F;
    int firstChild = -1;
    int secondChild = -1;

    Array<DockTabSpec> tabs{};
    int selectedTab = 0;

    Utf8String panelId{};
    bool passthroughInput = false;
};

/**
 * Serializable tree of split / tab / leaf nodes consumed by <c>DockWorkspace</c>.
 * Node indices are stable handles referenced by parent nodes and tab specs.
 */
class DockLayoutModel {
public:
    [[nodiscard]] int AddNode(DockNode node);
    void SetRoot(int nodeIndex) noexcept { rootIndex_ = nodeIndex; }
    [[nodiscard]] int GetRoot() const noexcept { return rootIndex_; }

    [[nodiscard]] const Array<DockNode>& GetNodes() const noexcept { return nodes_; }
    [[nodiscard]] DockNode* GetNode(int index) noexcept;
    [[nodiscard]] const DockNode* GetNode(int index) const noexcept;

    [[nodiscard]] int AddLeaf(Utf8String panelId, bool passthroughInput = false);
    [[nodiscard]] int AddSplit(
            DockSplitAxis axis,
            DockSplitMeasure measure,
            float splitValue,
            int firstChild,
            int secondChild);
    [[nodiscard]] int AddTabs(Array<DockTabSpec> tabs, int selectedTab = 0);

    /**
     * Default game-editor chrome:
     * [sidebar tabs | [viewport passthrough | inspector]].
     */
    [[nodiscard]] static DockLayoutModel CreateEditorDefault(
            float sidebarWidthPx = 300.0F,
            float viewportSplit = 0.72F);

    /** Reads the root split when it uses <c>LeadingPixels</c> on a horizontal axis. */
    [[nodiscard]] float GetRootLeadingPixels() const noexcept;

private:
    Array<DockNode> nodes_{};
    int rootIndex_ = -1;
};

}  // namespace Spark::Gui
