#include "spark/gui/DockLayoutModel.hpp"

#include <algorithm>

namespace Spark::Gui {

int DockLayoutModel::AddNode(DockNode node) {
    const int index = static_cast<int>(nodes.GetSize());
    nodes.PushBack(MoveTemp(node));
    return index;
}

DockNode* DockLayoutModel::GetNode(const int index) noexcept {
    if (index < 0 || static_cast<std::size_t>(index) >= nodes.GetSize()) {
        return nullptr;
    }
    return &nodes[static_cast<std::size_t>(index)];
}

const DockNode* DockLayoutModel::GetNode(const int index) const noexcept {
    if (index < 0 || static_cast<std::size_t>(index) >= nodes.GetSize()) {
        return nullptr;
    }
    return &nodes[static_cast<std::size_t>(index)];
}

int DockLayoutModel::AddLeaf(const Utf8String panelId, const bool passthroughInput) {
    DockNode node{};
    node.kind = DockNodeKind::Leaf;
    node.panelId = panelId;
    node.passthroughInput = passthroughInput;
    return AddNode(MoveTemp(node));
}

int DockLayoutModel::AddSplit(
        const DockSplitAxis axis,
        const DockSplitMeasure measure,
        const float splitValue,
        const int firstChild,
        const int secondChild) {
    DockNode node{};
    node.kind = DockNodeKind::Split;
    node.axis = axis;
    node.measure = measure;
    node.splitValue = splitValue;
    node.firstChild = firstChild;
    node.secondChild = secondChild;
    return AddNode(MoveTemp(node));
}

int DockLayoutModel::AddTabs(Array<DockTabSpec> tabs, const int selectedTab) {
    DockNode node{};
    node.kind = DockNodeKind::Tabs;
    node.tabs = MoveTemp(tabs);
    node.selectedTab = selectedTab;
    return AddNode(MoveTemp(node));
}

DockLayoutModel DockLayoutModel::CreateEditorDefault(const float sidebarWidthPx, const float viewportSplit) {
    DockLayoutModel model;
    const int viewport = model.AddLeaf(Utf8String("viewport"), true);
    const int inspector = model.AddLeaf(Utf8String("inspector"), false);
    const float clampedSplit = std::clamp(viewportSplit, 0.08F, 0.92F);
    const int center = model.AddSplit(
            DockSplitAxis::Horizontal, DockSplitMeasure::Fraction, clampedSplit, viewport, inspector);

    Array<DockTabSpec> leftTabs;
    {
        DockTabSpec scene{};
        scene.title = Utf8String("Scene");
        scene.panelId = Utf8String("hierarchy");
        leftTabs.PushBack(MoveTemp(scene));
    }
    {
        DockTabSpec project{};
        project.title = Utf8String("Project");
        project.panelId = Utf8String("project");
        leftTabs.PushBack(MoveTemp(project));
    }
    const int tabs = model.AddTabs(MoveTemp(leftTabs), 0);
    const int root = model.AddSplit(
            DockSplitAxis::Horizontal,
            DockSplitMeasure::LeadingPixels,
            sidebarWidthPx,
            tabs,
            center);
    model.SetRoot(root);
    return model;
}

float DockLayoutModel::GetRootLeadingPixels() const noexcept {
    const DockNode* root = GetNode(rootIndex);
    if (root == nullptr || root->kind != DockNodeKind::Split
            || root->measure != DockSplitMeasure::LeadingPixels) {
        return 300.0F;
    }
    return root->splitValue;
}

}  // namespace Spark::Gui
