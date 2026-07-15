#include "spark/editor/panels/ProjectBrowserPanel.hpp"

#include "spark/editor/EditorProject.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/controls/Label.hpp"
#include "spark/gui/controls/Panel.hpp"
#include "spark/gui/controls/StackPanel.hpp"

#include <cstdio>

namespace Spark::Editor {

ProjectBrowserPanel::ProjectBrowserPanel() {
    const Gui::GuiTheme& th = Gui::GuiTheme::SceneEditorDark();

    auto shell = MakeUnique<Gui::Panel>();
    shell->SetPadding(4.0F);
    shell->SetBackgroundEnabled(false);
    shell->SetChromeEnabled(false);
    shell->SetDropShadowEnabled(false);

    auto stack = MakeUnique<Gui::StackPanel>();
    auto body = MakeUnique<Gui::Label>();
    body->SetText(Utf8String("No project open."));
    body->SetFontSize(16.0F);
    body->SetTextColor(th.labelMuted);
    body_ = body.Get();
    stack->AddChild(MoveTemp(body));

    shell->AddChild(MoveTemp(stack));
    root_.Reset(shell.Release());
}

void ProjectBrowserPanel::OnAttach(EditorContext& ctx) {
    project_ = ctx.project;
}

void ProjectBrowserPanel::OnTick(const FrameTiming& /*timing*/, EditorContext& /*ctx*/) {
    if (body_ == nullptr || project_ == nullptr) {
        return;
    }
    if (!project_->IsOpen()) {
        body_->SetText(Utf8String("No project open."));
        return;
    }
    const EditorProjectSettings& s = project_->GetSettings();
    char buf[384]{};
    std::snprintf(
            buf,
            sizeof(buf),
            "%s\n%s\nworkspace: %s",
            s.projectName.CStr(),
            s.rootDirectory.CStr(),
            s.workspace == WorkspaceDimension::TwoD ? "2D" : "3D");
    body_->SetText(Utf8String(buf));
}

}  // namespace Spark::Editor
