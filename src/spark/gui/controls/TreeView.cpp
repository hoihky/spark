#include "spark/gui/controls/TreeView.hpp"

#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/gui/controls/Button.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace Spark::Gui {

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

}  // namespace

TreeView::TreeView() = default;

void TreeView::Clear() {
    nodes.Clear();
    nodeChildren.Clear();
    visibleRows.Clear();
    selectedNodeId = -1;
    scrollY = 0.0F;
    ClearChildren();
}

int TreeView::AddItem(const int parentIndex, Utf8String label) {
    if (parentIndex >= 0 && static_cast<std::size_t>(parentIndex) >= nodes.GetSize()) {
        return -1;
    }
    TreeViewItem it{};
    it.label = Spark::MoveTemp(label);
    it.parent = parentIndex;
    it.expanded = true;
    const int id = static_cast<int>(nodes.GetSize());
    nodes.PushBack(Spark::MoveTemp(it));
    RebuildVisibleRows();
    RebuildButtons();
    return id;
}

void TreeView::RebuildChildrenMap() {
    nodeChildren.Clear();
    nodeChildren.Resize(nodes.GetSize());
    for (std::size_t i = 0; i < nodes.GetSize(); ++i) {
        const int p = nodes[i].parent;
        if (p >= 0 && static_cast<std::size_t>(p) < nodeChildren.GetSize()) {
            nodeChildren[static_cast<std::size_t>(p)].PushBack(static_cast<int>(i));
        }
    }
}

void TreeView::VisitNode(const int id, const int depth) {
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

void TreeView::RebuildVisibleRows() {
    visibleRows.Clear();
    RebuildChildrenMap();
    for (std::size_t i = 0; i < nodes.GetSize(); ++i) {
        if (nodes[i].parent < 0) {
            VisitNode(static_cast<int>(i), 0);
        }
    }
}

bool TreeView::NodeHasChildren(const int nodeId) const noexcept {
    if (nodeId < 0 || static_cast<std::size_t>(nodeId) >= nodeChildren.GetSize()) {
        return false;
    }
    return !nodeChildren[static_cast<std::size_t>(nodeId)].IsEmpty();
}

void TreeView::ToggleExpanded(const int nodeId) {
    if (nodeId < 0 || static_cast<std::size_t>(nodeId) >= nodes.GetSize()) {
        return;
    }
    nodes[static_cast<std::size_t>(nodeId)].expanded = !nodes[static_cast<std::size_t>(nodeId)].expanded;
    RebuildVisibleRows();
    RebuildButtons();
}

void TreeView::RebuildButtons() {
    ClearChildren();
    for (std::size_t ri = 0; ri < visibleRows.GetSize(); ++ri) {
        const VisibleRow& vr = visibleRows[ri];
        const Utf8String lab = PrefixLabel(vr.depth, NodeHasChildren(vr.nodeId),
                nodes[static_cast<std::size_t>(vr.nodeId)].expanded,
                nodes[static_cast<std::size_t>(vr.nodeId)].label);
        auto b = MakeUnique<Button>();
        Button* raw = b.Get();
        raw->SetLabel(lab);
        raw->SetFontSize(itemFontPx);
        raw->SetOpaqueSurface(true);
        raw->SetAccentSelected(vr.nodeId == selectedNodeId);
        const int row = static_cast<int>(ri);
        const int nid = vr.nodeId;
        const int depth = vr.depth;
        const bool hasKids = NodeHasChildren(nid);
        raw->SetOnClickWithFrame([this, row, nid, depth, hasKids](const GuiFrameInput& fin, GuiCanvasComponent&) {
            if (row < 0 || static_cast<std::size_t>(row) >= visibleRows.GetSize()) {
                return;
            }
            float localX = fin.mouseX;
            const auto& wch = GetChildren();
            if (static_cast<std::size_t>(row) < wch.GetSize() && wch[static_cast<std::size_t>(row)]) {
                localX = fin.mouseX - wch[static_cast<std::size_t>(row)]->GetBounds().x;
            }
            selectedNodeId = nid;
            const bool expandClick = hasKids && localX < ExpandGlyphHitWidthPx(depth, itemFontPx);
            if (expandClick) {
                ToggleExpanded(nid);
                if (onSelect) {
                    onSelect(selectedNodeId);
                }
                return;
            }
            for (std::size_t j = 0; j < wch.GetSize(); ++j) {
                if (Button* btn = dynamic_cast<Button*>(wch[j].Get())) {
                    btn->SetAccentSelected(visibleRows[j].nodeId == selectedNodeId);
                }
            }
            if (onSelect) {
                onSelect(selectedNodeId);
            }
        });
        AddChild(Spark::MoveTemp(b));
    }
}

bool TreeView::HitTrack(const float x, const float y) const noexcept {
    return trackRect.Contains(x, y);
}

bool TreeView::RowIntersectsViewport(const Rect& rowBounds) const noexcept {
    if (maxScroll <= 0.0F) {
        return true;
    }
    const float viewTop = bounds.y;
    const float viewBottom = bounds.y + bounds.height;
    return rowBounds.y + rowBounds.height > viewTop && rowBounds.y < viewBottom;
}

void TreeView::UpdateScrollFromThumbTop(const float thumbTopY) {
    if (maxScroll <= 0.0F || trackRect.height <= thumbRect.height) {
        scrollY = 0.0F;
        return;
    }
    const float travel = std::max(0.0F, trackRect.height - thumbRect.height);
    if (travel <= 1.0e-4F) {
        return;
    }
    const float t = (thumbTopY - trackRect.y) / travel;
    scrollY = std::clamp(t, 0.0F, 1.0F) * maxScroll;
}

void TreeView::ScrollRowIntoView(const int visibleRowIndex) noexcept {
    if (maxScroll <= 0.0F || visibleRowIndex < 0) {
        return;
    }
    const float rowTop = static_cast<float>(visibleRowIndex) * rowHeight;
    const float rowBottom = rowTop + rowHeight;
    const float viewTop = scrollY;
    const float viewBottom = scrollY + bounds.height;
    if (rowTop < viewTop) {
        scrollY = rowTop;
    } else if (rowBottom > viewBottom) {
        scrollY = rowBottom - bounds.height;
    }
    scrollY = std::clamp(scrollY, 0.0F, maxScroll);
}

void TreeView::Arrange(const Rect& r) {
    bounds = r;
    const std::size_t n = visibleRows.GetSize();
    contentHeight = rowHeight * static_cast<float>(n);
    maxScroll = std::max(0.0F, contentHeight - r.height);
    scrollY = std::clamp(scrollY, 0.0F, maxScroll);

    const float innerW = (maxScroll > 0.0F) ? std::max(0.0F, r.width - kTrackW) : r.width;
    const float viewportTop = r.y;

    const auto& wch = GetChildren();
    for (std::size_t i = 0; i < wch.GetSize(); ++i) {
        if (wch[i]) {
            const float y = viewportTop - scrollY + static_cast<float>(i) * rowHeight;
            wch[i]->Arrange({r.x, y, innerW, rowHeight});
        }
    }

    if (maxScroll <= 0.0F) {
        trackRect = {0.0F, 0.0F, 0.0F, 0.0F};
        thumbRect = {0.0F, 0.0F, 0.0F, 0.0F};
    } else {
        trackRect = {r.x + innerW, r.y, kTrackW, r.height};
        constexpr float kMinThumb = 22.0F;
        const float thumbH =
                std::max(kMinThumb, r.height * (r.height / std::max(contentHeight, r.height)));
        const float travel = std::max(0.0F, trackRect.height - thumbH);
        const float thumbY = trackRect.y + (maxScroll > 0.0F ? (scrollY / maxScroll) * travel : 0.0F);
        thumbRect = {trackRect.x + 1.0F, thumbY, trackRect.width - 2.0F, thumbH};
    }
}

void TreeView::Paint(GuiPaintContext& ctx) const {
    if (!IsVisible()) {
        return;
    }
    if (maxScroll <= 0.0F) {
        Widget::Paint(ctx);
        return;
    }
    const float innerW = std::max(0.0F, bounds.width - kTrackW);
    const float clipX = std::floor(bounds.x);
    const float clipY = std::floor(bounds.y);
    const float clipR = std::ceil(bounds.x + innerW);
    const float clipB = std::ceil(bounds.y + bounds.height);
    ctx.PushClipRect(clipX, clipY, std::max(1.0F, clipR - clipX), std::max(1.0F, clipB - clipY));
    PaintChildren(ctx);
    ctx.PopClipRect();
    const GuiTheme& th = ctx.GetTheme();
    ctx.FillRect(trackRect.x, trackRect.y, trackRect.width, trackRect.height, th.insetTrackRgb, 0.92F);
    ctx.FillRectGradientVertical(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, th.thumbGradientTop,
            th.thumbGradientBottom, 0.95F);
    ctx.StrokeRect(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, 1.0F, th.borderRgb, 0.5F);
}

Widget* TreeView::FindDeepestHover(const float x, const float y) {
    if (!IsVisible() || !enabled) {
        return nullptr;
    }
    if (maxScroll > 0.0F && HitTrack(x, y)) {
        return this;
    }
    const float innerW = std::max(0.0F, bounds.width - (maxScroll > 0.0F ? kTrackW : 0.0F));
    const Rect content{bounds.x, bounds.y, innerW, bounds.height};
    if (!content.Contains(x, y)) {
        return nullptr;
    }
    const auto& ch = GetChildren();
    for (std::size_t i = ch.GetSize(); i > 0U; --i) {
        Widget* c = ch[i - 1U].Get();
        if (c == nullptr || !c->IsVisible()) {
            continue;
        }
        if (!RowIntersectsViewport(c->GetBounds())) {
            continue;
        }
        if (Widget* hit = c->FindDeepestHover(x, y)) {
            return hit;
        }
    }
    if (hitTest && content.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

void TreeView::NotifyPointerDown(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled) {
        return;
    }
    if (HitTrack(in.mouseX, in.mouseY)) {
        if (thumbRect.Contains(in.mouseX, in.mouseY)) {
            draggingThumb = true;
            grabOffsetY = in.mouseY - thumbRect.y;
        } else {
            draggingThumb = true;
            grabOffsetY = thumbRect.height * 0.5F;
            UpdateScrollFromThumbTop(in.mouseY - grabOffsetY);
        }
    }
}

void TreeView::NotifyPointerDrag(const GuiFrameInput& in, GuiCanvasComponent&) {
    if (!enabled || !draggingThumb) {
        return;
    }
    UpdateScrollFromThumbTop(in.mouseY - grabOffsetY);
}

void TreeView::NotifyPointerUp(const GuiFrameInput&, GuiCanvasComponent&) {
    draggingThumb = false;
}

void TreeView::ApplyScrollWheelDelta(const float deltaY) noexcept {
    if (deltaY == 0.0F || maxScroll <= 0.0F) {
        return;
    }
    static constexpr float kPixelsPerWheelUnit = 48.0F;
    scrollY = std::clamp(scrollY - deltaY * kPixelsPerWheelUnit, 0.0F, maxScroll);
}

void TreeView::ProcessKeyInput(IInput& input) {
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
    const auto& wch = GetChildren();
    for (std::size_t j = 0; j < wch.GetSize(); ++j) {
        if (Button* btn = dynamic_cast<Button*>(wch[j].Get())) {
            btn->SetAccentSelected(visibleRows[j].nodeId == selectedNodeId);
        }
    }
    ScrollRowIntoView(selRow);
    if (onSelect) {
        onSelect(selectedNodeId);
    }
}

}  // namespace Spark::Gui
