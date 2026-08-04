#include "spark/ui/spark/controls/SparkTreeView.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/core/IUiRenderer.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"

namespace Spark::Ui {

namespace {

Utf8String PrefixLabel(const int depth, const bool hasKids, const bool expanded, const Utf8String& body) {
    Utf8String out;
    for (int d = 0; d < depth; ++d) {
        out.AppendUtf8("   ");
    }
    if (hasKids) {
        out.AppendUtf8(expanded ? "▼ " : "▶ ");
    } else {
        out.AppendUtf8("  ");
    }
    out.AppendUtf8(body);
    return out;
}

float ExpandGlyphHitWidthPx(const int depth, const float itemFontPx) noexcept {
    const float charW = itemFontPx * 0.52F;
    return static_cast<float>(depth * 3 + 2) * charW;
}

float Clampf(const float v, const float lo, const float hi) {
    return std::max(lo, std::min(v, hi));
}

void TreeRowClickThunk(void* userData, const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (userData != nullptr) {
        auto* binding = static_cast<SparkTreeView::RowBinding*>(userData);
        if (binding->tree != nullptr) {
            binding->tree->HandleRowClick(binding->row, binding->nodeId, binding->depth, binding->hasKids, input);
        }
    }
}

}  // namespace

SparkTreeView::SparkTreeView(const TreeViewDesc& desc)
    : UiElementBase(desc.id), rowHeight(desc.rowHeight), itemFontPx(desc.itemFontSize) {}

float SparkTreeView::ScaledRowHeight() const noexcept {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    return metrics.Scaled(rowHeight > 0.0F ? rowHeight : metrics.listRowHeight);
}

float SparkTreeView::ScaledItemFontPx() const noexcept {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    return metrics.Scaled(itemFontPx > 0.0F ? itemFontPx : metrics.fontControl);
}

void SparkTreeView::Clear() {
    nodes.Clear();
    nodeChildren.Clear();
    visibleRows.Clear();
    rowBindings.Clear();
    selectedNodeId = -1;
    scrollY = 0.0F;
    children.Clear();
}

int SparkTreeView::AddItem(const int parentIndex, Utf8String label) {
    if (parentIndex >= 0 && static_cast<std::size_t>(parentIndex) >= nodes.GetSize()) {
        return -1;
    }
    TreeViewItem it{};
    it.label = MoveTemp(label);
    it.parent = parentIndex;
    it.expanded = true;
    const int id = static_cast<int>(nodes.GetSize());
    nodes.PushBack(MoveTemp(it));
    RebuildVisibleRows();
    RebuildButtons();
    return id;
}

void SparkTreeView::RebuildChildrenMap() {
    nodeChildren.Clear();
    nodeChildren.Resize(nodes.GetSize());
    for (std::size_t i = 0; i < nodes.GetSize(); ++i) {
        const int p = nodes[i].parent;
        if (p >= 0 && static_cast<std::size_t>(p) < nodeChildren.GetSize()) {
            nodeChildren[static_cast<std::size_t>(p)].PushBack(static_cast<int>(i));
        }
    }
}

void SparkTreeView::VisitNode(const int id, const int depth) {
    if (id < 0 || static_cast<std::size_t>(id) >= nodes.GetSize()) {
        return;
    }
    visibleRows.PushBack(VisibleRow{id, depth});
    if (!nodes[static_cast<std::size_t>(id)].expanded) {
        return;
    }
    if (static_cast<std::size_t>(id) >= nodeChildren.GetSize()) {
        return;
    }
    const Array<int>& ch = nodeChildren[static_cast<std::size_t>(id)];
    for (std::size_t j = 0; j < ch.GetSize(); ++j) {
        VisitNode(ch[j], depth + 1);
    }
}

void SparkTreeView::RebuildVisibleRows() {
    visibleRows.Clear();
    RebuildChildrenMap();
    for (std::size_t i = 0; i < nodes.GetSize(); ++i) {
        if (nodes[i].parent < 0) {
            VisitNode(static_cast<int>(i), 0);
        }
    }
}

bool SparkTreeView::NodeHasChildren(const int nodeId) const noexcept {
    if (nodeId < 0 || static_cast<std::size_t>(nodeId) >= nodeChildren.GetSize()) {
        return false;
    }
    return !nodeChildren[static_cast<std::size_t>(nodeId)].IsEmpty();
}

void SparkTreeView::ToggleExpanded(const int nodeId) {
    if (nodeId < 0 || static_cast<std::size_t>(nodeId) >= nodes.GetSize()) {
        return;
    }
    nodes[static_cast<std::size_t>(nodeId)].expanded = !nodes[static_cast<std::size_t>(nodeId)].expanded;
    RebuildVisibleRows();
    RebuildButtons();
}

void SparkTreeView::RebuildButtons() {
    rowBindings.Clear();
    children.Clear();
    rowBindings.Reserve(visibleRows.GetSize());
    for (std::size_t ri = 0; ri < visibleRows.GetSize(); ++ri) {
        const VisibleRow& vr = visibleRows[ri];
        const Utf8String lab = PrefixLabel(vr.depth, NodeHasChildren(vr.nodeId),
                nodes[static_cast<std::size_t>(vr.nodeId)].expanded,
                nodes[static_cast<std::size_t>(vr.nodeId)].label);
        ButtonDesc desc{};
        desc.id = Utf8String("tree-row");
        desc.label = lab;
        auto button = MakeUnique<SparkButton>(desc);
        button->SetFontSize(itemFontPx > 0.0F ? itemFontPx : GetActiveUiLayoutMetrics().fontControl);
        button->SetOpaqueSurface(true);
        button->SetAccentSelected(vr.nodeId == selectedNodeId);
        RowBinding binding{};
        binding.tree = this;
        binding.row = static_cast<int>(ri);
        binding.nodeId = vr.nodeId;
        binding.depth = vr.depth;
        binding.hasKids = NodeHasChildren(vr.nodeId);
        rowBindings.PushBack(binding);
        UiFrameVoidCallback click{};
        click.fn = &TreeRowClickThunk;
        click.userData = &rowBindings[rowBindings.GetSize() - 1U];
        button->SetOnClickWithFrame(click);
        AddChild(UniquePtr<IUiElement>(static_cast<IUiElement*>(button.Release())));
    }
}

void SparkTreeView::HandleRowClick(
        const int row, const int nodeId, const int depth, const bool hasKids, const UiFrameInput& input) {
    if (row < 0 || static_cast<std::size_t>(row) >= visibleRows.GetSize()) {
        return;
    }
    float localX = input.mouseX;
    if (static_cast<std::size_t>(row) < children.GetSize() && children[static_cast<std::size_t>(row)] != nullptr) {
        localX = input.mouseX - children[static_cast<std::size_t>(row)]->GetBounds().x;
    }
    selectedNodeId = nodeId;
    const float fontPx = ScaledItemFontPx();
    const bool expandClick = hasKids && localX < ExpandGlyphHitWidthPx(depth, fontPx);
    if (expandClick) {
        ToggleExpanded(nodeId);
        onSelect.Invoke(selectedNodeId);
        return;
    }
    for (std::size_t j = 0; j < children.GetSize(); ++j) {
        if (auto* btn = dynamic_cast<SparkButton*>(children[j].Get())) {
            btn->SetAccentSelected(visibleRows[j].nodeId == selectedNodeId);
        }
    }
    onSelect.Invoke(selectedNodeId);
}

bool SparkTreeView::HitTrack(const float x, const float y) const noexcept {
    return trackRect.Contains(x, y);
}

bool SparkTreeView::RowIntersectsViewport(const Rect& rowBounds) const noexcept {
    if (maxScroll <= 0.0F) {
        return true;
    }
    const float overflowTop = std::min(scrollY, bounds.y);
    const float viewTop = bounds.y - overflowTop;
    const float viewBottom = bounds.y + bounds.height;
    return rowBounds.y + rowBounds.height > viewTop && rowBounds.y < viewBottom;
}

void SparkTreeView::UpdateScrollFromThumbTop(const float thumbTopY) {
    if (maxScroll <= 0.0F || trackRect.height <= thumbRect.height) {
        scrollY = 0.0F;
        return;
    }
    const float travel = std::max(0.0F, trackRect.height - thumbRect.height);
    if (travel <= 1.0e-4F) {
        return;
    }
    const float t = (thumbTopY - trackRect.y) / travel;
    scrollY = Clampf(t, 0.0F, 1.0F) * maxScroll;
}

void SparkTreeView::ScrollRowIntoView(const int visibleRowIndex) noexcept {
    if (maxScroll <= 0.0F || visibleRowIndex < 0) {
        return;
    }
    const float scaledRowH = ScaledRowHeight();
    const float rowTop = static_cast<float>(visibleRowIndex) * scaledRowH;
    const float rowBottom = rowTop + scaledRowH;
    const float viewTop = scrollY;
    const float viewBottom = scrollY + bounds.height;
    if (rowTop < viewTop) {
        scrollY = rowTop;
    } else if (rowBottom > viewBottom) {
        scrollY = rowBottom - bounds.height;
    }
    scrollY = Clampf(scrollY, 0.0F, maxScroll);
}

void SparkTreeView::SyncScrollLayout() noexcept {
    if (arrangeRect.width > 0.0F && arrangeRect.height > 0.0F) {
        Arrange(arrangeRect);
    }
}

void SparkTreeView::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const float scaledRowH = ScaledRowHeight();
    outDesired.width = Clampf(GetActiveUiLayoutMetrics().Scaled(200.0F), constraints.minWidth, constraints.maxWidth);
    outDesired.height = Clampf(scaledRowH * static_cast<float>((std::max)(visibleRows.GetSize(), std::size_t{1})),
            constraints.minHeight,
            constraints.maxHeight);
}

void SparkTreeView::Arrange(const Rect& r) {
    arrangeRect = r;
    bounds = r;
    if (!visible || r.width < 0.5F || r.height < 0.5F) {
        maxScroll = 0.0F;
        scrollY = 0.0F;
        trackRect = {};
        thumbRect = {};
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i] != nullptr) {
                children[i]->Arrange(Rect{});
            }
        }
        return;
    }

    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float scaledRowH = ScaledRowHeight();
    const float trackW = metrics.Scaled(kTrackWidth);
    const std::size_t n = visibleRows.GetSize();
    contentHeight = scaledRowH * static_cast<float>(n);
    maxScroll = std::max(0.0F, contentHeight - r.height);
    scrollY = Clampf(scrollY, 0.0F, maxScroll);

    const float innerW = (maxScroll > 0.0F) ? std::max(0.0F, r.width - trackW) : r.width;
    const float viewportTop = r.y;

    for (std::size_t i = 0; i < n; ++i) {
        if (i < children.GetSize() && children[i] != nullptr) {
            const float y = viewportTop - scrollY + static_cast<float>(i) * scaledRowH;
            children[i]->Arrange({r.x, y, innerW, scaledRowH});
        }
    }

    if (maxScroll <= 0.0F) {
        trackRect = {};
        thumbRect = {};
    } else {
        trackRect = {r.x + innerW, r.y, trackW, r.height};
        const float kMinThumb = metrics.Scaled(22.0F);
        const float thumbH =
                std::max(kMinThumb, r.height * (r.height / std::max(contentHeight, r.height)));
        const float travel = std::max(0.0F, trackRect.height - thumbH);
        const float thumbY = trackRect.y + (maxScroll > 0.0F ? (scrollY / maxScroll) * travel : 0.0F);
        thumbRect = {trackRect.x + 1.0F, thumbY, trackRect.width - 2.0F, thumbH};
    }
}

void SparkTreeView::Paint(IUiRenderer& renderer) {
    if (!visible || bounds.height < 0.5F || bounds.width < 0.5F) {
        return;
    }
    if (maxScroll <= 0.0F) {
        UiElementBase::Paint(renderer);
        return;
    }

    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const UiTheme& theme = renderer.GetTheme();
    const float trackW = metrics.Scaled(kTrackWidth);
    const float innerW = std::max(0.0F, bounds.width - trackW);
    const float clipX = std::floor(bounds.x);
    const float clipY = std::floor(bounds.y);
    const float clipR = std::ceil(bounds.x + innerW);
    const float clipB = std::ceil(bounds.y + bounds.height);
    const float clipW = std::max(1.0F, clipR - clipX);
    const float clipH = std::max(1.0F, clipB - clipY);
    const float overflowTop = std::min(scrollY, clipY);
    const float fillY = clipY - overflowTop;
    const float fillH = clipH + overflowTop;

    renderer.FillRectGradientVertical(
            clipX, fillY, clipW, fillH, theme.scrollViewportTop, theme.scrollViewportBottom, theme.scrollViewportAlpha);

    renderer.PushClip(Rect{clipX, fillY, clipW, fillH});
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] == nullptr || !children[i]->IsVisible()) {
            continue;
        }
        if (!RowIntersectsViewport(children[i]->GetBounds())) {
            continue;
        }
        children[i]->Paint(renderer);
    }
    renderer.PopClip();

    renderer.FillRect(trackRect.x, trackRect.y, trackRect.width, trackRect.height, theme.insetTrackRgb, 0.92F);
    renderer.FillRoundRectGradientVertical(
            thumbRect.x,
            thumbRect.y,
            thumbRect.width,
            thumbRect.height,
            metrics.Scaled(3.0F),
            theme.thumbGradientTop,
            theme.thumbGradientBottom,
            0.95F);
    renderer.StrokeRect(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, 1.0F, theme.borderRgb, 0.5F);
}

IUiElement* SparkTreeView::HitTest(const float x, const float y) {
    if (!WantsHitTest() || bounds.height < 0.5F || bounds.width < 0.5F) {
        return nullptr;
    }
    if (maxScroll > 0.0F && HitTrack(x, y)) {
        return this;
    }
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    const float trackW = metrics.Scaled(kTrackWidth);
    const float innerW = (maxScroll > 0.0F) ? std::max(0.0F, bounds.width - trackW) : bounds.width;
    const float overflowTop = std::min(scrollY, bounds.y);
    const Rect content{bounds.x, bounds.y - overflowTop, innerW, bounds.height + overflowTop};
    if (!content.Contains(x, y)) {
        return nullptr;
    }
    for (std::size_t i = children.GetSize(); i > 0; --i) {
        IUiElement* child = children[i - 1U].Get();
        if (child == nullptr || !child->IsVisible()) {
            continue;
        }
        if (!RowIntersectsViewport(child->GetBounds())) {
            continue;
        }
        if (IUiElement* hit = child->HitTest(x, y)) {
            return hit;
        }
    }
    return this;
}

void SparkTreeView::OnPointerDown(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!enabled) {
        return;
    }
    if (HitTrack(input.mouseX, input.mouseY)) {
        draggingThumb = true;
        grabOffsetY = thumbRect.Contains(input.mouseX, input.mouseY) ? input.mouseY - thumbRect.y : thumbRect.height * 0.5F;
        UpdateScrollFromThumbTop(input.mouseY - grabOffsetY);
        SyncScrollLayout();
    }
}

void SparkTreeView::OnPointerDrag(const UiFrameInput& input, UiCanvasComponent& /*canvas*/) {
    if (!enabled || !draggingThumb) {
        return;
    }
    UpdateScrollFromThumbTop(input.mouseY - grabOffsetY);
    SyncScrollLayout();
}

void SparkTreeView::OnPointerUp(const UiFrameInput& /*input*/, UiCanvasComponent& /*canvas*/) {
    draggingThumb = false;
}

void SparkTreeView::OnScroll(const float /*deltaX*/, const float deltaY) {
    ApplyScrollWheelDelta(deltaY);
}

void SparkTreeView::ApplyScrollWheelDelta(const float deltaY) noexcept {
    if (deltaY == 0.0F || maxScroll <= 0.0F) {
        return;
    }
    const float scaledRowH = ScaledRowHeight();
    const int steps = static_cast<int>(std::round(deltaY));
    if (steps == 0) {
        return;
    }
    scrollY = Clampf(scrollY - static_cast<float>(steps) * scaledRowH, 0.0F, maxScroll);
    SyncScrollLayout();
}

void SparkTreeView::ProcessKeyInput(IInput& input) {
    if (!enabled || visibleRows.IsEmpty()) {
        return;
    }
    int selRow = 0;
    for (std::size_t i = 0; i < visibleRows.GetSize(); ++i) {
        if (visibleRows[i].nodeId == selectedNodeId) {
            selRow = static_cast<int>(i);
            break;
        }
    }
    if (selectedNodeId < 0) {
        selRow = 0;
    }
    const int last = static_cast<int>(visibleRows.GetSize()) - 1;
    if (input.IsKeyPressedThisFrame(GLFW_KEY_UP)) {
        selRow = std::max(0, selRow - 1);
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_DOWN)) {
        selRow = std::min(last, selRow + 1);
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_HOME)) {
        selRow = 0;
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_END)) {
        selRow = last;
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_RIGHT)) {
        const int nid = visibleRows[static_cast<std::size_t>(selRow)].nodeId;
        if (NodeHasChildren(nid) && !nodes[static_cast<std::size_t>(nid)].expanded) {
            ToggleExpanded(nid);
        }
        return;
    } else if (input.IsKeyPressedThisFrame(GLFW_KEY_LEFT)) {
        const int nid = visibleRows[static_cast<std::size_t>(selRow)].nodeId;
        if (NodeHasChildren(nid) && nodes[static_cast<std::size_t>(nid)].expanded) {
            ToggleExpanded(nid);
        }
        return;
    } else {
        return;
    }
    selectedNodeId = visibleRows[static_cast<std::size_t>(selRow)].nodeId;
    for (std::size_t j = 0; j < children.GetSize(); ++j) {
        if (auto* btn = dynamic_cast<SparkButton*>(children[j].Get())) {
            btn->SetAccentSelected(visibleRows[j].nodeId == selectedNodeId);
        }
    }
    ScrollRowIntoView(selRow);
    onSelect.Invoke(selectedNodeId);
}

}  // namespace Spark::Ui
