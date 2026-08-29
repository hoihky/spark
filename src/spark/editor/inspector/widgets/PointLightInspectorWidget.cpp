#include "spark/editor/inspector/widgets/PointLightInspectorWidget.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ui/spark/UiChild.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"

namespace Spark::Editor {

bool PointLightInspectorWidget::IsRelevantFor(const GameObject* const target) const noexcept {
    return target != nullptr && target->HasComponent<PointLightComponent>();
}

void PointLightInspectorWidget::BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) {
    if (built) {
        return;
    }

    Ui::PanelDesc sectionDesc{};
    sectionDesc.id = Utf8String("inspector_point_light_section");
    sectionDesc.title = Utf8String("Point Light");
    auto sectionUp = factory.CreatePanel(sectionDesc);
    sectionPanel = sectionUp.Get();

    InspectorUiBuilder::AddSectionHeader(*sectionPanel, factory, "Point Light");

    Ui::CheckBoxDesc enabledDesc{};
    enabledDesc.id = Utf8String("inspector_point_light_enabled");
    enabledDesc.label = Utf8String("Enabled");
    enabledDesc.value = true;
    auto enabledUp = factory.CreateCheckBox(enabledDesc);
    enabledBox = enabledUp.Get();
    Ui::UiBoolCallback enabledCb{};
    enabledCb.fn = OnEnabledCheckboxChanged;
    enabledCb.userData = this;
    enabledBox->SetOnChanged(enabledCb);
    AdoptUiChild(*sectionPanel, MoveTemp(enabledUp));

    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_point_light_r", "Color R", 1.0F, 0.0F, 1.0F,
            OnColorRChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_point_light_g", "Color G", 0.92F, 0.0F, 1.0F,
            OnColorGChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_point_light_b", "Color B", 0.82F, 0.0F, 1.0F,
            OnColorBChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_point_light_intensity", "Intensity", 2.4F, 0.0F, 20.0F,
            OnIntensitySliderChanged, this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_point_light_range", "Range", 12.0F, 0.1F, 200.0F,
            OnRangeSliderChanged, this);

    AdoptUiChild(parent, MoveTemp(sectionUp));
    built = true;
}

void PointLightInspectorWidget::SetSectionVisible(const bool visible) {
    InspectorUiBuilder::SetVisible(sectionPanel, visible);
}

void PointLightInspectorWidget::SyncFromTarget(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
    if (!built || suppressSync || editTracker.HasPendingEdit()) {
        return;
    }
    if (!IsRelevantFor(ctx.target)) {
        return;
    }
    liveState = SetPointLightCommand::Capture(*ctx.target);
    if (enabledBox != nullptr) {
        enabledBox->SetValue(liveState.enabled);
    }
    InspectorUiBuilder::SetSliderValue(bindings[0].slider, liveState.color.x);
    InspectorUiBuilder::SetSliderValue(bindings[1].slider, liveState.color.y);
    InspectorUiBuilder::SetSliderValue(bindings[2].slider, liveState.color.z);
    InspectorUiBuilder::SetSliderValue(bindings[3].slider, liveState.intensity);
    InspectorUiBuilder::SetSliderValue(bindings[4].slider, liveState.range);
}

void PointLightInspectorWidget::CommitPendingEdits(const InspectorWidgetContext& ctx) {
    if (!IsRelevantFor(ctx.target) || !ctx.CanEdit()) {
        editTracker.Cancel();
        return;
    }
    editTracker.TryCommit(liveState, [&](const PointLightState& before, const PointLightState& after) {
        if (SetPointLightCommand::NearlyEqual(before, after) || ctx.commandStack == nullptr) {
            return;
        }
        auto command = MakeUnique<SetPointLightCommand>(*ctx.target, before, after);
        ctx.commandStack->Record(UniquePtr<IEditorCommand>(command.Release()));
    });
}

void PointLightInspectorWidget::CancelPendingEdits() {
    editTracker.Cancel();
}

void PointLightInspectorWidget::PrepareFrameContext(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
}

void PointLightInspectorWidget::ApplyLiveEdit() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (PointLightComponent* light = activeCtx.target->GetComponent<PointLightComponent>()) {
        suppressSync = true;
        light->SetColor(liveState.color);
        light->SetIntensity(liveState.intensity);
        light->SetRange(liveState.range);
        light->SetEnabled(liveState.enabled);
        suppressSync = false;
    }
}

void PointLightInspectorWidget::OnColorChanged(const int axis, const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetPointLightCommand::Capture(*activeCtx.target));
    if (axis == 0) {
        liveState.color.x = value;
    } else if (axis == 1) {
        liveState.color.y = value;
    } else {
        liveState.color.z = value;
    }
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void PointLightInspectorWidget::OnIntensityChanged(const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetPointLightCommand::Capture(*activeCtx.target));
    liveState.intensity = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void PointLightInspectorWidget::OnRangeChanged(const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetPointLightCommand::Capture(*activeCtx.target));
    liveState.range = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void PointLightInspectorWidget::OnEnabledChanged(const bool value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetPointLightCommand::Capture(*activeCtx.target));
    liveState.enabled = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void PointLightInspectorWidget::OnColorRChanged(void* const userData, const float value) {
    static_cast<PointLightInspectorWidget*>(userData)->OnColorChanged(0, value);
}

void PointLightInspectorWidget::OnColorGChanged(void* const userData, const float value) {
    static_cast<PointLightInspectorWidget*>(userData)->OnColorChanged(1, value);
}

void PointLightInspectorWidget::OnColorBChanged(void* const userData, const float value) {
    static_cast<PointLightInspectorWidget*>(userData)->OnColorChanged(2, value);
}

void PointLightInspectorWidget::OnIntensitySliderChanged(void* const userData, const float value) {
    static_cast<PointLightInspectorWidget*>(userData)->OnIntensityChanged(value);
}

void PointLightInspectorWidget::OnRangeSliderChanged(void* const userData, const float value) {
    static_cast<PointLightInspectorWidget*>(userData)->OnRangeChanged(value);
}

void PointLightInspectorWidget::OnEnabledCheckboxChanged(void* const userData, const bool value) {
    static_cast<PointLightInspectorWidget*>(userData)->OnEnabledChanged(value);
}

}  // namespace Spark::Editor
