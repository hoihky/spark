#include "spark/editor/inspector/widgets/MeshInspectorWidget.hpp"

#include "spark/editor/commands/SetMeshAlbedoCommand.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ui/spark/UiChild.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"

namespace Spark::Editor {

bool MeshInspectorWidget::IsRelevantFor(const GameObject* const target) const noexcept {
    return target != nullptr && target->HasComponent<MeshComponent>();
}

void MeshInspectorWidget::BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) {
    if (built) {
        return;
    }

    bindings.Reserve(4U);

    Ui::PanelDesc sectionDesc{};
    sectionDesc.id = Utf8String("inspector_mesh_section");
    sectionDesc.title = Utf8String("Mesh");
    auto sectionUp = factory.CreatePanel(sectionDesc);
    sectionPanel = sectionUp.Get();

    InspectorUiBuilder::AddSectionHeader(*sectionPanel, factory, "Mesh");
    Ui::LabelDesc infoDesc{};
    infoDesc.id = Utf8String("inspector_mesh_info");
    infoDesc.text = Utf8String("Mesh");
    infoDesc.muted = true;
    auto infoUp = factory.CreateLabel(infoDesc);
    infoLabel = infoUp.Get();
    AdoptUiChild(*sectionPanel, MoveTemp(infoUp));

    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_mesh_r", "Albedo R", 1.0F, 0.0F, 1.0F, OnAlbedoRChanged,
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_mesh_g", "Albedo G", 1.0F, 0.0F, 1.0F, OnAlbedoGChanged,
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_mesh_b", "Albedo B", 1.0F, 0.0F, 1.0F, OnAlbedoBChanged,
            this);

    AdoptUiChild(parent, MoveTemp(sectionUp));
    built = true;
}

void MeshInspectorWidget::SetSectionVisible(const bool visible) {
    if (sectionPanel != nullptr) {
        sectionPanel->SetVisible(visible);
    }
}

void MeshInspectorWidget::SyncFromTarget(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
    if (!built || suppressSync || editTracker.HasPendingEdit()) {
        return;
    }
    if (!IsRelevantFor(ctx.target)) {
        return;
    }
    if (const MeshComponent* mesh = ctx.target->GetComponent<MeshComponent>()) {
        liveAlbedo = mesh->GetAlbedo();
        if (infoLabel != nullptr) {
            const SharedPtr<Mesh>& asset = mesh->GetMesh();
            if (asset) {
                infoLabel->SetText(asset->GetName());
            } else {
                infoLabel->SetText(Utf8String("Built-in mesh slot"));
            }
        }
        InspectorUiBuilder::SetSliderValue(bindings[0].slider, liveAlbedo.x);
        InspectorUiBuilder::SetSliderValue(bindings[1].slider, liveAlbedo.y);
        InspectorUiBuilder::SetSliderValue(bindings[2].slider, liveAlbedo.z);
    }
}

void MeshInspectorWidget::CommitPendingEdits(const InspectorWidgetContext& ctx) {
    if (!IsRelevantFor(ctx.target) || !ctx.CanEdit()) {
        editTracker.Cancel();
        return;
    }
    editTracker.TryCommit(liveAlbedo, [&](const Vector3& before, const Vector3& after) {
        if (SetMeshAlbedoCommand::NearlyEqual(before, after) || ctx.commandStack == nullptr) {
            return;
        }
        auto command = MakeUnique<SetMeshAlbedoCommand>(*ctx.target, before, after);
        ctx.commandStack->Record(UniquePtr<IEditorCommand>(command.Release()));
    });
}

void MeshInspectorWidget::CancelPendingEdits() {
    editTracker.Cancel();
}

void MeshInspectorWidget::PrepareFrameContext(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
}

void MeshInspectorWidget::ApplyLiveEdit() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (MeshComponent* mesh = activeCtx.target->GetComponent<MeshComponent>()) {
        suppressSync = true;
        mesh->SetAlbedo(liveAlbedo);
        suppressSync = false;
    }
}

void MeshInspectorWidget::OnAlbedoChanged(const int axis, const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (MeshComponent* mesh = activeCtx.target->GetComponent<MeshComponent>()) {
        editTracker.BeginEdit(mesh->GetAlbedo());
    }
    if (axis == 0) {
        liveAlbedo.x = value;
    } else if (axis == 1) {
        liveAlbedo.y = value;
    } else {
        liveAlbedo.z = value;
    }
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void MeshInspectorWidget::OnAlbedoRChanged(void* const userData, const float value) {
    static_cast<MeshInspectorWidget*>(userData)->OnAlbedoChanged(0, value);
}

void MeshInspectorWidget::OnAlbedoGChanged(void* const userData, const float value) {
    static_cast<MeshInspectorWidget*>(userData)->OnAlbedoChanged(1, value);
}

void MeshInspectorWidget::OnAlbedoBChanged(void* const userData, const float value) {
    static_cast<MeshInspectorWidget*>(userData)->OnAlbedoChanged(2, value);
}

}  // namespace Spark::Editor
