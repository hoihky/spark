#include "spark/editor/inspector/widgets/TransformInspectorWidget.hpp"

#include "spark/editor/commands/SetTransformCommand.hpp"
#include "spark/editor/inspector/InspectorMath.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ui/spark/UiChild.hpp"

namespace Spark::Editor {

bool TransformInspectorWidget::IsRelevantFor(const GameObject* const target) const noexcept {
    return target != nullptr && target->HasComponent<TransformComponent>();
}

void TransformInspectorWidget::BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) {
    if (built) {
        return;
    }

    bindings.Reserve(12U);

    Ui::PanelDesc sectionDesc{};
    sectionDesc.id = Utf8String("inspector_transform_section");
    sectionDesc.title = Utf8String("Transform");
    auto sectionUp = factory.CreatePanel(sectionDesc);
    sectionPanel = sectionUp.Get();

    InspectorUiBuilder::AddSectionHeader(*sectionPanel, factory, "Transform");
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pos_x", "Position X", 0.0F, -100.0F, 100.0F,
            OnPositionXChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pos_y", "Position Y", 0.0F, -100.0F, 100.0F,
            OnPositionYChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pos_z", "Position Z", 0.0F, -100.0F, 100.0F,
            OnPositionZChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_rot_x", "Rotation X", 0.0F, -180.0F, 180.0F,
            OnRotationXChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_rot_y", "Rotation Y", 0.0F, -180.0F, 180.0F,
            OnRotationYChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_rot_z", "Rotation Z", 0.0F, -180.0F, 180.0F,
            OnRotationZChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_scale_x", "Scale X", 1.0F, 0.01F, 10.0F, OnScaleXChanged,
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_scale_y", "Scale Y", 1.0F, 0.01F, 10.0F, OnScaleYChanged,
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_scale_z", "Scale Z", 1.0F, 0.01F, 10.0F, OnScaleZChanged,
            this);

    AdoptUiChild(parent, MoveTemp(sectionUp));
    built = true;
}

void TransformInspectorWidget::SetSectionVisible(const bool visible) {
    if (sectionPanel != nullptr) {
        sectionPanel->SetVisible(visible);
    }
}

void TransformInspectorWidget::SyncFromTarget(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
    const bool externalSync = ctx.externalTransformEditActive && ctx.target != nullptr &&
            ctx.target == ctx.externalTransformTarget;
    if (!built || suppressSync) {
        return;
    }
    if (!externalSync && editTracker.HasPendingEdit()) {
        return;
    }
    if (!IsRelevantFor(ctx.target)) {
        return;
    }
    if (const TransformComponent* transform = ctx.target->GetComponent<TransformComponent>()) {
        liveTransform = transform->GetLocalTransform();
        liveEulerDegrees = QuaternionToEulerDegrees(liveTransform.rotation);
        InspectorUiBuilder::SetSliderValue(bindings[0].slider, liveTransform.translation.x);
        InspectorUiBuilder::SetSliderValue(bindings[1].slider, liveTransform.translation.y);
        InspectorUiBuilder::SetSliderValue(bindings[2].slider, liveTransform.translation.z);
        InspectorUiBuilder::SetSliderValue(bindings[3].slider, liveEulerDegrees.x);
        InspectorUiBuilder::SetSliderValue(bindings[4].slider, liveEulerDegrees.y);
        InspectorUiBuilder::SetSliderValue(bindings[5].slider, liveEulerDegrees.z);
        InspectorUiBuilder::SetSliderValue(bindings[6].slider, liveTransform.scale.x);
        InspectorUiBuilder::SetSliderValue(bindings[7].slider, liveTransform.scale.y);
        InspectorUiBuilder::SetSliderValue(bindings[8].slider, liveTransform.scale.z);
    }
}

void TransformInspectorWidget::CommitPendingEdits(const InspectorWidgetContext& ctx) {
    if (!IsRelevantFor(ctx.target) || !ctx.CanEdit()) {
        editTracker.Cancel();
        return;
    }
    liveTransform.rotation = EulerDegreesToQuaternion(liveEulerDegrees);
    editTracker.TryCommit(liveTransform, [&](const Transform& before, const Transform& after) {
        if (SetTransformCommand::NearlyEqual(before, after) || ctx.commandStack == nullptr) {
            return;
        }
        auto command = MakeUnique<SetTransformCommand>(*ctx.target, before, after);
        ctx.commandStack->Record(UniquePtr<IEditorCommand>(command.Release()));
    });
}

void TransformInspectorWidget::CancelPendingEdits() {
    editTracker.Cancel();
}

void TransformInspectorWidget::PrepareFrameContext(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
}

void TransformInspectorWidget::ApplyLiveEdit() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (TransformComponent* transform = activeCtx.target->GetComponent<TransformComponent>()) {
        suppressSync = true;
        liveTransform.rotation = EulerDegreesToQuaternion(liveEulerDegrees);
        transform->SetLocalTransform(liveTransform);
        suppressSync = false;
    }
}

void TransformInspectorWidget::OnPositionChanged(const int axis, const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (TransformComponent* transform = activeCtx.target->GetComponent<TransformComponent>()) {
        editTracker.BeginEdit(transform->GetLocalTransform());
    }
    if (axis == 0) {
        liveTransform.translation.x = value;
    } else if (axis == 1) {
        liveTransform.translation.y = value;
    } else {
        liveTransform.translation.z = value;
    }
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void TransformInspectorWidget::OnRotationChanged(const int axis, const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (TransformComponent* transform = activeCtx.target->GetComponent<TransformComponent>()) {
        editTracker.BeginEdit(transform->GetLocalTransform());
    }
    if (axis == 0) {
        liveEulerDegrees.x = value;
    } else if (axis == 1) {
        liveEulerDegrees.y = value;
    } else {
        liveEulerDegrees.z = value;
    }
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void TransformInspectorWidget::OnScaleChanged(const int axis, const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (TransformComponent* transform = activeCtx.target->GetComponent<TransformComponent>()) {
        editTracker.BeginEdit(transform->GetLocalTransform());
    }
    if (axis == 0) {
        liveTransform.scale.x = value;
    } else if (axis == 1) {
        liveTransform.scale.y = value;
    } else {
        liveTransform.scale.z = value;
    }
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void TransformInspectorWidget::OnPositionXChanged(void* const userData, const float value) {
    static_cast<TransformInspectorWidget*>(userData)->OnPositionChanged(0, value);
}

void TransformInspectorWidget::OnPositionYChanged(void* const userData, const float value) {
    static_cast<TransformInspectorWidget*>(userData)->OnPositionChanged(1, value);
}

void TransformInspectorWidget::OnPositionZChanged(void* const userData, const float value) {
    static_cast<TransformInspectorWidget*>(userData)->OnPositionChanged(2, value);
}

void TransformInspectorWidget::OnRotationXChanged(void* const userData, const float value) {
    static_cast<TransformInspectorWidget*>(userData)->OnRotationChanged(0, value);
}

void TransformInspectorWidget::OnRotationYChanged(void* const userData, const float value) {
    static_cast<TransformInspectorWidget*>(userData)->OnRotationChanged(1, value);
}

void TransformInspectorWidget::OnRotationZChanged(void* const userData, const float value) {
    static_cast<TransformInspectorWidget*>(userData)->OnRotationChanged(2, value);
}

void TransformInspectorWidget::OnScaleXChanged(void* const userData, const float value) {
    static_cast<TransformInspectorWidget*>(userData)->OnScaleChanged(0, value);
}

void TransformInspectorWidget::OnScaleYChanged(void* const userData, const float value) {
    static_cast<TransformInspectorWidget*>(userData)->OnScaleChanged(1, value);
}

void TransformInspectorWidget::OnScaleZChanged(void* const userData, const float value) {
    static_cast<TransformInspectorWidget*>(userData)->OnScaleChanged(2, value);
}

}  // namespace Spark::Editor
