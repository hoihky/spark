#include "spark/editor/panels/InspectorPanel.hpp"

#include "spark/editor/EditorSelection.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/MeshComponent.hpp"
#include "spark/ecs/components/PointLightComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/controls/Panel.hpp"
#include "spark/gui/controls/StackPanel.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdio>
#include <cstring>

namespace Spark::Editor {

InspectorPanel::InspectorPanel() {
    const Gui::GuiTheme& th = Gui::GuiTheme::SceneEditorDark();

    auto shell = MakeUnique<Gui::Panel>();
    shell->SetPadding(4.0F);
    shell->SetBackgroundEnabled(false);
    shell->SetChromeEnabled(false);
    shell->SetDropShadowEnabled(false);

    auto stack = MakeUnique<Gui::StackPanel>();
    stack->SetSpacing(6.0F);

    auto titleUp = MakeUnique<Gui::Label>();
    titleUp->SetText(Utf8String("Inspector"));
    titleUp->SetFontSize(20.0F);
    titleUp->SetTextColor(th.labelPrimary);
    title = titleUp.Get();
    stack->AddChild(MoveTemp(titleUp));

    auto bodyUp = MakeUnique<Gui::Label>();
    bodyUp->SetText(Utf8String("Select an entity in the Hierarchy."));
    bodyUp->SetFontSize(16.0F);
    bodyUp->SetTextColor(th.labelMuted);
    body = bodyUp.Get();
    stack->AddChild(MoveTemp(bodyUp));

    shell->AddChild(MoveTemp(stack));
    root.Reset(shell.Release());
}

void InspectorPanel::OnAttach(EditorContext& ctx) {
    selection = ctx.selection;
}

void InspectorPanel::OnTick(const FrameTiming& /*timing*/, EditorContext& /*ctx*/) {
    if (body == nullptr || selection == nullptr) {
        return;
    }
    GameObject* obj = selection->GetPrimary();
    if (obj == nullptr) {
        body->SetText(Utf8String("Select an entity in the Hierarchy."));
        return;
    }

    char buf[512]{};
  std::snprintf(buf, sizeof(buf), "Name: %s\n", obj->GetName().CStr());

    if (TransformComponent* tr = obj->GetComponent<TransformComponent>()) {
        const Vector3 p = tr->GetLocalTransform().translation;
        char line[128]{};
        std::snprintf(line, sizeof(line), "Position: %.2f, %.2f, %.2f\n", p.x, p.y, p.z);
        std::strncat(buf, line, sizeof(buf) - std::strlen(buf) - 1);
    }
    if (obj->GetComponent<MeshComponent>() != nullptr) {
        std::strncat(buf, "MeshComponent\n", sizeof(buf) - std::strlen(buf) - 1);
    }
    if (obj->GetComponent<PointLightComponent>() != nullptr) {
        std::strncat(buf, "PointLightComponent\n", sizeof(buf) - std::strlen(buf) - 1);
    }
    body->SetText(Utf8String(buf));
}

}  // namespace Spark::Editor
