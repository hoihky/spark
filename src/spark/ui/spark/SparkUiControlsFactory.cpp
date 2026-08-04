#include "spark/ui/spark/SparkUiControlsFactory.hpp"

#include "spark/ui/spark/controls/SparkControls.hpp"
#include "spark/ui/spark/controls/SparkList.hpp"
#include "spark/ui/spark/controls/SparkMultiSelectList.hpp"
#include "spark/ui/spark/controls/SparkScrollPanel.hpp"
#include "spark/ui/spark/controls/SparkTreeView.hpp"

namespace Spark::Ui {

UniquePtr<IUiButton> SparkUiControlsFactory::CreateButton(const ButtonDesc& desc) {
    return UniquePtr<IUiButton>(new SparkButton(desc));
}

UniquePtr<ISlider> SparkUiControlsFactory::CreateSlider(const SliderDesc& desc) {
    return UniquePtr<ISlider>(new SparkSlider(desc));
}

UniquePtr<ICheckBox> SparkUiControlsFactory::CreateCheckBox(const CheckBoxDesc& desc) {
    return UniquePtr<ICheckBox>(new SparkCheckBox(desc));
}

UniquePtr<IPanel> SparkUiControlsFactory::CreatePanel(const PanelDesc& desc) {
    return UniquePtr<IPanel>(new SparkPanel(desc));
}

UniquePtr<ILabel> SparkUiControlsFactory::CreateLabel(const LabelDesc& desc) {
    return UniquePtr<ILabel>(new SparkLabel(desc));
}

UniquePtr<ISeparator> SparkUiControlsFactory::CreateSeparator(const SeparatorDesc& desc) {
    return UniquePtr<ISeparator>(new SparkSeparator(desc));
}

UniquePtr<IScrollPanel> SparkUiControlsFactory::CreateScrollPanel(const ScrollPanelDesc& desc) {
    return UniquePtr<IScrollPanel>(new SparkScrollPanel(desc));
}

UniquePtr<IDockWorkspace> SparkUiControlsFactory::CreateDockWorkspace(const DockWorkspaceDesc& desc) {
    return UniquePtr<IDockWorkspace>(new SparkDockWorkspace(desc));
}

UniquePtr<IList> SparkUiControlsFactory::CreateList(const ListDesc& desc) {
    return UniquePtr<IList>(new SparkList(desc));
}

UniquePtr<IMultiSelectList> SparkUiControlsFactory::CreateMultiSelectList(const MultiSelectListDesc& desc) {
    return UniquePtr<IMultiSelectList>(new SparkMultiSelectList(desc));
}

UniquePtr<ITreeView> SparkUiControlsFactory::CreateTreeView(const TreeViewDesc& desc) {
    return UniquePtr<ITreeView>(new SparkTreeView(desc));
}

}  // namespace Spark::Ui
