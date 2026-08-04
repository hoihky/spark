#pragma once

#include "spark/ui/factory/IUiControlsFactory.hpp"

namespace Spark::Ui {

class SparkUiControlsFactory final : public IUiControlsFactory {
public:
    UniquePtr<IUiButton> CreateButton(const ButtonDesc& desc) override;
    UniquePtr<ISlider> CreateSlider(const SliderDesc& desc) override;
    UniquePtr<ICheckBox> CreateCheckBox(const CheckBoxDesc& desc) override;
    UniquePtr<IPanel> CreatePanel(const PanelDesc& desc) override;
    UniquePtr<ILabel> CreateLabel(const LabelDesc& desc) override;
    UniquePtr<ISeparator> CreateSeparator(const SeparatorDesc& desc) override;
    UniquePtr<IScrollPanel> CreateScrollPanel(const ScrollPanelDesc& desc) override;
    UniquePtr<IDockWorkspace> CreateDockWorkspace(const DockWorkspaceDesc& desc) override;
    UniquePtr<IList> CreateList(const ListDesc& desc) override;
    UniquePtr<IMultiSelectList> CreateMultiSelectList(const MultiSelectListDesc& desc) override;
    UniquePtr<ITreeView> CreateTreeView(const TreeViewDesc& desc) override;
};

}  // namespace Spark::Ui
