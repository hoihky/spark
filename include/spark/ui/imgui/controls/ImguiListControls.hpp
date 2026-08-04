#pragma once

#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/factory/ControlDesc.hpp"

namespace Spark::Ui {

class ImguiList final : public IList, public UiElementBase {
public:
    explicit ImguiList(const ListDesc& desc);

    void SetItems(Array<Utf8String> itemsIn) override;
    [[nodiscard]] int GetSelectedIndex() const noexcept override { return selectedIndex; }
    void SetSelectedIndex(int index) override;
    void SetOnSelectionChanged(UiIntCallback handler) override { onSelect = handler; }
    void SetScrollY(float y) override { scrollY = y; }
    [[nodiscard]] float GetScrollY() const noexcept override { return scrollY; }
    void ScrollToTop() noexcept override { scrollY = 0.0F; }

    void Paint(IUiRenderer& renderer) override;

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;

private:
    float listHeight = 240.0F;
    bool fillRemainingHeight = false;
    Array<Utf8String> items{};
    int selectedIndex = -1;
    UiIntCallback onSelect{};
    float scrollY = 0.0F;
};

class ImguiMultiSelectList final : public IMultiSelectList, public UiElementBase {
public:
    explicit ImguiMultiSelectList(const MultiSelectListDesc& desc);

    void SetItems(Array<Utf8String> itemsIn) override;
    [[nodiscard]] const Array<int>& GetSelectedIndices() const noexcept override { return selected; }
    void SetSelectedIndices(Array<int> indices) override;
    void SetOnSelectionChanged(UiVoidCallback handler) override { onSelect = handler; }
    void SetScrollY(float y) override { scrollY = y; }
    [[nodiscard]] float GetScrollY() const noexcept override { return scrollY; }

    void Paint(IUiRenderer& renderer) override;

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;

private:
    void ToggleSelection(int index, bool range, bool toggle);
    float listHeight = 240.0F;
    Array<Utf8String> items{};
    Array<int> selected{};
    int anchorIndex = 0;
    UiVoidCallback onSelect{};
    float scrollY = 0.0F;
};

struct ImguiTreeNode {
    Utf8String label{};
    int parent = -1;
    bool expanded = true;
};

class ImguiTreeView final : public ITreeView, public UiElementBase {
public:
    explicit ImguiTreeView(const TreeViewDesc& desc);

    void Clear() override;
    int AddItem(int parentIndex, Utf8String label) override;
    [[nodiscard]] int GetSelectedNodeId() const noexcept override { return selectedNodeId; }
    void SetOnSelectionChanged(UiIntCallback handler) override { onSelect = handler; }

    void Paint(IUiRenderer& renderer) override;

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;

private:
    void PaintSubtree(int nodeId);
    float treeHeight = 280.0F;
    Array<ImguiTreeNode> nodes{};
    int selectedNodeId = -1;
    UiIntCallback onSelect{};
};

}  // namespace Spark::Ui
