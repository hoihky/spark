#include "spark/editor/panels/ProjectBrowserPanel.hpp"

#include "spark/ui/Ui.hpp"
#include "spark/ui/runtime/IUiBackend.hpp"
#include "spark/ui/spark/UiChild.hpp"

namespace Spark::Editor {

ProjectBrowserPanel::ProjectBrowserPanel() = default;

void ProjectBrowserPanel::EnsureBuilt() {
    if (built) {
        return;
    }
    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    Ui::PanelDesc shellDesc{};
    shellDesc.id = Utf8String("project_shell");
    shellDesc.title = Utf8String("Assets");
    auto shell = factory.CreatePanel(shellDesc);

    if (assetCatalog != nullptr && assetHost != nullptr) {
        assetBrowser.BuildInto(*shell, factory, *assetCatalog, *assetHost);
    } else {
        Ui::LabelDesc bodyDesc{};
        bodyDesc.id = Utf8String("project_body");
        bodyDesc.text = Utf8String("Asset catalog not bound.");
        bodyDesc.muted = true;
        Ui::AdoptUiChild(*shell, factory.CreateLabel(bodyDesc));
    }

    root.Reset(static_cast<Ui::IUiElement*>(shell.Release()));
    built = true;
}

void ProjectBrowserPanel::OnAttach(EditorContext& ctx) {
    assetHost = ctx.assetHost;
    EnsureBuilt();
}

void ProjectBrowserPanel::OnTick(const FrameTiming& /*timing*/, EditorContext& /*ctx*/) {}

}  // namespace Spark::Editor
