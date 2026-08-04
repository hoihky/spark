#include "spark/editor/panels/ProjectBrowserPanel.hpp"

#include "spark/editor/EditorProject.hpp"
#include "spark/ui/Ui.hpp"
#include "spark/ui/spark/UiChild.hpp"

#include <cstdio>

namespace Spark::Editor {

ProjectBrowserPanel::ProjectBrowserPanel() = default;

void ProjectBrowserPanel::EnsureBuilt() {
    if (built) {
        return;
    }
    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    Ui::PanelDesc shellDesc{};
    shellDesc.id = Utf8String("project_shell");
    shellDesc.title = Utf8String("Project");
    auto shell = factory.CreatePanel(shellDesc);

    Ui::LabelDesc bodyDesc{};
    bodyDesc.id = Utf8String("project_body");
    bodyDesc.text = Utf8String("No project open.");
    bodyDesc.muted = true;
    auto bodyUp = factory.CreateLabel(bodyDesc);
    body = bodyUp.Get();
    AdoptUiChild(*shell, MoveTemp(bodyUp));

    root.Reset(static_cast<Ui::IUiElement*>(shell.Release()));
    built = true;
}

void ProjectBrowserPanel::OnAttach(EditorContext& ctx) {
    EnsureBuilt();
    project = ctx.project;
}

void ProjectBrowserPanel::OnTick(const FrameTiming& /*timing*/, EditorContext& /*ctx*/) {
    if (body == nullptr || project == nullptr) {
        return;
    }
    if (!project->IsOpen()) {
        body->SetText(Utf8String("No project open."));
        return;
    }
    const EditorProjectSettings& s = project->GetSettings();
    char buf[384]{};
    std::snprintf(
            buf,
            sizeof(buf),
            "%s\n%s\nworkspace: %s",
            s.projectName.CStr(),
            s.rootDirectory.CStr(),
            s.workspace == WorkspaceDimension::TwoD ? "2D" : "3D");
    body->SetText(Utf8String(buf));
}

}  // namespace Spark::Editor
