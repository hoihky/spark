#include "spark/editor/panels/InspectorPanel.hpp"

#include "spark/editor/EditorSelection.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/ui/Ui.hpp"
#include "spark/ui/spark/UiChild.hpp"

#include <cstdio>
#include <cstring>

namespace Spark::Editor {

InspectorPanel::InspectorPanel() = default;

void InspectorPanel::EnsureBuilt() {
    if (built) {
        return;
    }
    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    Ui::PanelDesc shellDesc{};
    shellDesc.id = Utf8String("inspector_shell");
    shellDesc.title = Utf8String("Inspector");
    auto shell = factory.CreatePanel(shellDesc);

    Ui::LabelDesc titleDesc{};
    titleDesc.id = Utf8String("inspector_title");
    titleDesc.text = Utf8String("Inspector");
    auto titleUp = factory.CreateLabel(titleDesc);
    title = titleUp.Get();
    AdoptUiChild(*shell, MoveTemp(titleUp));

    Ui::LabelDesc bodyDesc{};
    bodyDesc.id = Utf8String("inspector_body");
    bodyDesc.text = Utf8String("Select an entity in the Hierarchy.");
    bodyDesc.muted = true;
    auto bodyUp = factory.CreateLabel(bodyDesc);
    body = bodyUp.Get();
    AdoptUiChild(*shell, MoveTemp(bodyUp));

    root.Reset(static_cast<Ui::IUiElement*>(shell.Release()));
    built = true;
}

void InspectorPanel::OnAttach(EditorContext& ctx) {
    EnsureBuilt();
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
