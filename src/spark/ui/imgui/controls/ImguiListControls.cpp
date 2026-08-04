#include "spark/ui/imgui/controls/ImguiListControls.hpp"

#include <algorithm>

#include "spark/config.hpp"
#include "spark/ui/core/ImguiUiRenderer.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace Spark::Ui {

namespace {

[[nodiscard]] ImguiUiRenderer* AsImgui(IUiRenderer& renderer) noexcept {
    return dynamic_cast<ImguiUiRenderer*>(&renderer);
}

float ClampMeasure(const UiMeasureConstraints& constraints, const float desired) {
    float v = desired;
    if (constraints.maxWidth > 0.0F) {
        v = std::min(v, constraints.maxWidth);
    }
    if (constraints.maxHeight > 0.0F) {
        v = std::min(v, constraints.maxHeight);
    }
    return std::max(v, constraints.minHeight > 0.0F ? constraints.minHeight : v);
}

}  // namespace

ImguiList::ImguiList(const ListDesc& desc)
    : UiElementBase(desc.id)
    , listHeight(desc.rowHeight > 0.0F ? desc.rowHeight * 8.0F : 240.0F)
    , fillRemainingHeight(desc.fillRemainingHeight) {}

void ImguiList::SetItems(Array<Utf8String> itemsIn) {
    items = MoveTemp(itemsIn);
}

void ImguiList::SetSelectedIndex(const int index) {
    selectedIndex = index;
}

void ImguiList::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    outDesired.width = ClampMeasure(constraints, metrics.Scaled(200.0F));
    outDesired.height = ClampMeasure(constraints, metrics.Scaled(listHeight));
}

void ImguiList::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    ImguiUiRenderer* imgui = AsImgui(renderer);
    if (imgui == nullptr) {
        return;
    }
#if SPARK_ENABLE_IMGUI
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    float height = metrics.Scaled(listHeight);
    if (fillRemainingHeight) {
        const float avail = ImGui::GetContentRegionAvail().y;
        if (avail > 0.0F) {
            height = avail;
        }
    }
    ImGui::PushID(GetId().CStr());
    if (ImGui::BeginListBox("list", ImVec2(-1.0F, height))) {
        if (scrollY > 0.0F) {
            ImGui::SetScrollY(scrollY);
        }
        for (std::size_t i = 0; i < items.GetSize(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            const bool selected = static_cast<int>(i) == selectedIndex;
            if (ImGui::Selectable(items[i].CStr(), selected)) {
                selectedIndex = static_cast<int>(i);
                onSelect.Invoke(selectedIndex);
            }
            ImGui::PopID();
        }
        scrollY = ImGui::GetScrollY();
        ImGui::EndListBox();
    }
    ImGui::PopID();
#else
    (void)imgui;
#endif
}

ImguiMultiSelectList::ImguiMultiSelectList(const MultiSelectListDesc& desc)
    : UiElementBase(desc.id), listHeight(desc.rowHeight > 0.0F ? desc.rowHeight * 8.0F : 240.0F) {}

void ImguiMultiSelectList::SetItems(Array<Utf8String> itemsIn) {
    items = MoveTemp(itemsIn);
    selected.Clear();
}

void ImguiMultiSelectList::SetSelectedIndices(Array<int> indices) {
    selected = MoveTemp(indices);
    std::sort(selected.GetData(), selected.GetData() + selected.GetSize());
}

void ImguiMultiSelectList::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    outDesired.width = ClampMeasure(constraints, metrics.Scaled(200.0F));
    outDesired.height = ClampMeasure(constraints, metrics.Scaled(listHeight));
}

void ImguiMultiSelectList::ToggleSelection(const int index, const bool range, const bool toggle) {
    if (index < 0 || index >= static_cast<int>(items.GetSize())) {
        return;
    }
    if (range) {
        const int lo = std::min(anchorIndex, index);
        const int hi = std::max(anchorIndex, index);
        for (int i = lo; i <= hi; ++i) {
            bool found = false;
            for (std::size_t s = 0; s < selected.GetSize(); ++s) {
                if (selected[s] == i) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                selected.PushBack(i);
            }
        }
    } else if (toggle) {
        bool found = false;
        for (std::size_t s = 0; s < selected.GetSize(); ++s) {
            if (selected[s] == index) {
                selected.RemoveAt(s);
                found = true;
                break;
            }
        }
        if (!found) {
            selected.PushBack(index);
        }
        anchorIndex = index;
    } else {
        selected.Clear();
        selected.PushBack(index);
        anchorIndex = index;
    }
    std::sort(selected.GetData(), selected.GetData() + selected.GetSize());
    onSelect.Invoke();
}

void ImguiMultiSelectList::Paint(IUiRenderer& renderer) {
    if (!visible) {
        return;
    }
    ImguiUiRenderer* imgui = AsImgui(renderer);
    if (imgui == nullptr) {
        return;
    }
#if SPARK_ENABLE_IMGUI
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const float height = metrics.Scaled(listHeight);
    ImGui::PushID(GetId().CStr());
    if (ImGui::BeginListBox("mlist", ImVec2(-1.0F, height))) {
        if (scrollY > 0.0F) {
            ImGui::SetScrollY(scrollY);
        }
        for (std::size_t i = 0; i < items.GetSize(); ++i) {
            bool isSelected = false;
            for (std::size_t s = 0; s < selected.GetSize(); ++s) {
                if (selected[s] == static_cast<int>(i)) {
                    isSelected = true;
                    break;
                }
            }
            if (ImGui::Selectable(items[i].CStr(), isSelected)) {
                const bool ctrl = ImGui::GetIO().KeyCtrl;
                const bool shift = ImGui::GetIO().KeyShift;
                ToggleSelection(static_cast<int>(i), shift, ctrl);
            }
        }
        scrollY = ImGui::GetScrollY();
        ImGui::EndListBox();
    }
    ImGui::PopID();
#else
    (void)imgui;
#endif
}

ImguiTreeView::ImguiTreeView(const TreeViewDesc& desc)
    : UiElementBase(desc.id), treeHeight(desc.rowHeight > 0.0F ? desc.rowHeight * 10.0F : 280.0F) {}

void ImguiTreeView::Clear() {
    nodes.Clear();
    selectedNodeId = -1;
}

int ImguiTreeView::AddItem(const int parentIndex, Utf8String label) {
    if (parentIndex < -1 || parentIndex >= static_cast<int>(nodes.GetSize())) {
        return -1;
    }
    ImguiTreeNode node{};
    node.label = MoveTemp(label);
    node.parent = parentIndex;
    nodes.PushBack(node);
    return static_cast<int>(nodes.GetSize()) - 1;
}

void ImguiTreeView::DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) {
    const UiLayoutMetrics& metrics = GetActiveUiLayoutMetrics();
    outDesired.width = ClampMeasure(constraints, metrics.Scaled(220.0F));
    outDesired.height = ClampMeasure(constraints, metrics.Scaled(treeHeight));
}

void ImguiTreeView::PaintSubtree(const int nodeId) {
#if SPARK_ENABLE_IMGUI
    if (nodeId < 0 || nodeId >= static_cast<int>(nodes.GetSize())) {
        return;
    }
    ImguiTreeNode& node = nodes[static_cast<std::size_t>(nodeId)];
    bool hasChildren = false;
    for (std::size_t i = 0; i < nodes.GetSize(); ++i) {
        if (nodes[i].parent == nodeId) {
            hasChildren = true;
            break;
        }
    }
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (nodeId == selectedNodeId) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    const bool open = ImGui::TreeNodeEx(node.label.CStr(), flags);
    if (ImGui::IsItemClicked()) {
        selectedNodeId = nodeId;
        onSelect.Invoke(nodeId);
    }
    if (open && hasChildren) {
        for (std::size_t i = 0; i < nodes.GetSize(); ++i) {
            if (nodes[i].parent == nodeId) {
                PaintSubtree(static_cast<int>(i));
            }
        }
        ImGui::TreePop();
    }
#else
    (void)nodeId;
#endif
}

void ImguiTreeView::Paint(IUiRenderer& renderer) {
    if (!visible || AsImgui(renderer) == nullptr) {
        return;
    }
#if SPARK_ENABLE_IMGUI
    const UiLayoutMetrics& metrics = renderer.GetLayoutMetrics();
    const float height = metrics.Scaled(treeHeight);
    ImGui::PushID(GetId().CStr());
    if (ImGui::BeginChild("tree", ImVec2(-1.0F, height), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
        for (std::size_t i = 0; i < nodes.GetSize(); ++i) {
            if (nodes[i].parent == -1) {
                PaintSubtree(static_cast<int>(i));
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopID();
#endif
}

}  // namespace Spark::Ui
