#include "spark/ui/imgui/DearImguiControlsFactory.hpp"

#include "spark/ui/imgui/controls/ImguiControls.hpp"
#include "spark/ui/imgui/controls/ImguiListControls.hpp"

namespace Spark::Ui {

UniquePtr<IUiButton> DearImguiControlsFactory::CreateButton(const ButtonDesc& desc) {
    return UniquePtr<IUiButton>(new ImguiButton(desc));
}

UniquePtr<ISlider> DearImguiControlsFactory::CreateSlider(const SliderDesc& desc) {
    return UniquePtr<ISlider>(new ImguiSlider(desc));
}

UniquePtr<ICheckBox> DearImguiControlsFactory::CreateCheckBox(const CheckBoxDesc& desc) {
    return UniquePtr<ICheckBox>(new ImguiCheckBox(desc));
}

UniquePtr<IPanel> DearImguiControlsFactory::CreatePanel(const PanelDesc& desc) {
    return UniquePtr<IPanel>(new ImguiPanel(desc));
}

UniquePtr<ILabel> DearImguiControlsFactory::CreateLabel(const LabelDesc& desc) {
    return UniquePtr<ILabel>(new ImguiLabel(desc));
}

UniquePtr<ISeparator> DearImguiControlsFactory::CreateSeparator(const SeparatorDesc& desc) {
    return UniquePtr<ISeparator>(new ImguiSeparator(desc));
}

UniquePtr<IScrollPanel> DearImguiControlsFactory::CreateScrollPanel(const ScrollPanelDesc& desc) {
    return UniquePtr<IScrollPanel>(new ImguiScrollPanel(desc));
}

UniquePtr<IDockWorkspace> DearImguiControlsFactory::CreateDockWorkspace(const DockWorkspaceDesc& desc) {
    return UniquePtr<IDockWorkspace>(new ImguiDockWorkspace(desc));
}

UniquePtr<IList> DearImguiControlsFactory::CreateList(const ListDesc& desc) {
    return UniquePtr<IList>(new ImguiList(desc));
}

UniquePtr<IMultiSelectList> DearImguiControlsFactory::CreateMultiSelectList(const MultiSelectListDesc& desc) {
    return UniquePtr<IMultiSelectList>(new ImguiMultiSelectList(desc));
}

UniquePtr<ITreeView> DearImguiControlsFactory::CreateTreeView(const TreeViewDesc& desc) {
    return UniquePtr<ITreeView>(new ImguiTreeView(desc));
}

}  // namespace Spark::Ui
