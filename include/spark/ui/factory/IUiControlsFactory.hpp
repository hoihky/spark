#pragma once

#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/factory/ControlDesc.hpp"

namespace Spark::Ui {

/**
 * Abstract Factory: creates families of UI controls for a single backend (Spark native or Dear ImGui).
 */
class IUiControlsFactory {
public:
    virtual ~IUiControlsFactory() = default;

    virtual UniquePtr<IUiButton> CreateButton(const ButtonDesc& desc) = 0;
    virtual UniquePtr<ISlider> CreateSlider(const SliderDesc& desc) = 0;
    virtual UniquePtr<ICheckBox> CreateCheckBox(const CheckBoxDesc& desc) = 0;
    virtual UniquePtr<IPanel> CreatePanel(const PanelDesc& desc) = 0;
    virtual UniquePtr<ILabel> CreateLabel(const LabelDesc& desc) = 0;
    virtual UniquePtr<ISeparator> CreateSeparator(const SeparatorDesc& desc) = 0;
    virtual UniquePtr<IScrollPanel> CreateScrollPanel(const ScrollPanelDesc& desc) = 0;
    virtual UniquePtr<IDockWorkspace> CreateDockWorkspace(const DockWorkspaceDesc& desc) = 0;
    virtual UniquePtr<IList> CreateList(const ListDesc& desc) = 0;
    virtual UniquePtr<IMultiSelectList> CreateMultiSelectList(const MultiSelectListDesc& desc) = 0;
    virtual UniquePtr<ITreeView> CreateTreeView(const TreeViewDesc& desc) = 0;
};

}  // namespace Spark::Ui
