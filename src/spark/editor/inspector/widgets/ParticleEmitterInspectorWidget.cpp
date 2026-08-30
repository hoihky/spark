#include "spark/editor/inspector/widgets/ParticleEmitterInspectorWidget.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ui/spark/UiChild.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"

#include <cmath>

namespace Spark::Editor {

namespace {

constexpr int kSliderCount = 13;

}  // namespace

bool ParticleEmitterInspectorWidget::IsRelevantFor(const GameObject* const target) const noexcept {
    return target != nullptr && target->HasComponent<ParticleEmitterComponent>();
}

void ParticleEmitterInspectorWidget::BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) {
    if (built) {
        return;
    }

    bindings.Reserve(static_cast<std::size_t>(kSliderCount));

    Ui::PanelDesc sectionDesc{};
    sectionDesc.id = Utf8String("inspector_particle_emitter_section");
    sectionDesc.title = Utf8String("Particle Emitter");
    auto sectionUp = factory.CreatePanel(sectionDesc);
    sectionPanel = sectionUp.Get();

    InspectorUiBuilder::AddSectionHeader(*sectionPanel, factory, "Particle Emitter");

    Ui::CheckBoxDesc enabledDesc{};
    enabledDesc.id = Utf8String("inspector_particle_emitter_enabled");
    enabledDesc.label = Utf8String("Enabled");
    enabledDesc.value = true;
    auto enabledUp = factory.CreateCheckBox(enabledDesc);
    enabledBox = enabledUp.Get();
    Ui::UiBoolCallback enabledCb{};
    enabledCb.fn = OnEnabledChangedStatic;
    enabledCb.userData = this;
    enabledBox->SetOnChanged(enabledCb);
    AdoptUiChild(*sectionPanel, MoveTemp(enabledUp));

    Ui::CheckBoxDesc localDesc{};
    localDesc.id = Utf8String("inspector_particle_emitter_local");
    localDesc.label = Utf8String("Local Emission");
    localDesc.value = false;
    auto localUp = factory.CreateCheckBox(localDesc);
    localEmissionBox = localUp.Get();
    Ui::UiBoolCallback localCb{};
    localCb.fn = OnLocalEmissionChangedStatic;
    localCb.userData = this;
    localEmissionBox->SetOnChanged(localCb);
    AdoptUiChild(*sectionPanel, MoveTemp(localUp));

    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_rate", "Emission Rate", 48.0F, 0.0F, 256.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(0, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_life_min", "Life Min", 0.8F, 0.05F, 8.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(1, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_life_max", "Life Max", 1.6F, 0.05F, 8.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(2, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_size_start", "Size Start", 0.14F, 0.001F, 2.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(3, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_size_end", "Size End", 0.02F, 0.001F, 2.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(4, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_color_r", "Color Start R", 0.95F, 0.0F, 1.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(5, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_color_g", "Color Start G", 0.85F, 0.0F, 1.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(6, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_color_b", "Color Start B", 0.35F, 0.0F, 1.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(7, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_spread", "Spread", 0.55F, 0.0F, 3.14F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(8, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_speed_min", "Speed Min", 1.2F, 0.0F, 20.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(9, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_speed_max", "Speed Max", 2.8F, 0.0F, 20.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(10, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_module", "Module (0=cont 1=burst 2=ring)", 0.0F, 0.0F, 2.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(11, value); },
            this);
    InspectorUiBuilder::AddSlider(
            bindings, *sectionPanel, factory, "inspector_pe_ring_radius", "Ring Radius", 0.35F, 0.0F, 4.0F,
            [](void* userData, float value) { static_cast<ParticleEmitterInspectorWidget*>(userData)->OnSliderChanged(12, value); },
            this);

    AdoptUiChild(parent, MoveTemp(sectionUp));
    built = true;
}

void ParticleEmitterInspectorWidget::SetSectionVisible(const bool visible) {
    if (sectionPanel != nullptr) {
        sectionPanel->SetVisible(visible);
    }
}

void ParticleEmitterInspectorWidget::SyncFromTarget(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
    if (!built || suppressSync || editTracker.HasPendingEdit()) {
        return;
    }
    if (!IsRelevantFor(ctx.target)) {
        return;
    }
    liveState = SetParticleEmitterCommand::Capture(*ctx.target);
    if (enabledBox != nullptr) {
        enabledBox->SetValue(liveState.enabled);
    }
    if (localEmissionBox != nullptr) {
        localEmissionBox->SetValue(liveState.useLocalEmission);
    }
    InspectorUiBuilder::SetSliderValue(bindings[0].slider, liveState.emissionRate);
    InspectorUiBuilder::SetSliderValue(bindings[1].slider, liveState.lifeMin);
    InspectorUiBuilder::SetSliderValue(bindings[2].slider, liveState.lifeMax);
    InspectorUiBuilder::SetSliderValue(bindings[3].slider, liveState.sizeStart);
    InspectorUiBuilder::SetSliderValue(bindings[4].slider, liveState.sizeEnd);
    InspectorUiBuilder::SetSliderValue(bindings[5].slider, liveState.colorStart.x);
    InspectorUiBuilder::SetSliderValue(bindings[6].slider, liveState.colorStart.y);
    InspectorUiBuilder::SetSliderValue(bindings[7].slider, liveState.colorStart.z);
    InspectorUiBuilder::SetSliderValue(bindings[8].slider, liveState.spreadRadians);
    InspectorUiBuilder::SetSliderValue(bindings[9].slider, liveState.speedMin);
    InspectorUiBuilder::SetSliderValue(bindings[10].slider, liveState.speedMax);
    InspectorUiBuilder::SetSliderValue(bindings[11].slider, static_cast<float>(liveState.emissionModule));
    InspectorUiBuilder::SetSliderValue(bindings[12].slider, liveState.ringRadius);
}

void ParticleEmitterInspectorWidget::CommitPendingEdits(const InspectorWidgetContext& ctx) {
    if (!IsRelevantFor(ctx.target) || !ctx.CanEdit()) {
        editTracker.Cancel();
        return;
    }
    editTracker.TryCommit(liveState, [&](const ParticleEmitterState& before, const ParticleEmitterState& after) {
        if (SetParticleEmitterCommand::NearlyEqual(before, after) || ctx.commandStack == nullptr) {
            return;
        }
        auto command = MakeUnique<SetParticleEmitterCommand>(*ctx.target, before, after);
        ctx.commandStack->Record(UniquePtr<IEditorCommand>(command.Release()));
    });
}

void ParticleEmitterInspectorWidget::CancelPendingEdits() {
    editTracker.Cancel();
}

void ParticleEmitterInspectorWidget::PrepareFrameContext(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
}

void ParticleEmitterInspectorWidget::ApplyLiveEdit() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (ParticleEmitterComponent* emitter = activeCtx.target->GetComponent<ParticleEmitterComponent>()) {
        suppressSync = true;
        const Vector4 colorEnd = emitter->GetColorEnd();
        emitter->SetEmitterEnabled(liveState.enabled);
        emitter->SetEmissionRate(liveState.emissionRate);
        emitter->SetLifetime(liveState.lifeMin, liveState.lifeMax);
        emitter->SetStartEndSize(liveState.sizeStart, liveState.sizeEnd);
        liveState.colorStart.w = emitter->GetColorStart().w;
        emitter->SetStartEndColor(liveState.colorStart, colorEnd);
        emitter->SetSpreadAngleRadians(liveState.spreadRadians);
        emitter->SetSpeedRange(liveState.speedMin, liveState.speedMax);
        emitter->SetUseLocalEmission(liveState.useLocalEmission);
        emitter->SetEmissionModuleId(
                liveState.emissionModule == 1   ? "burst_only"
                : liveState.emissionModule == 2 ? "ring"
                                                  : "continuous");
        emitter->SetRingRadius(liveState.ringRadius);
        suppressSync = false;
    }
}

void ParticleEmitterInspectorWidget::OnSliderChanged(const int index, const float value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetParticleEmitterCommand::Capture(*activeCtx.target));
    switch (index) {
        case 0:
            liveState.emissionRate = value;
            break;
        case 1:
            liveState.lifeMin = value;
            if (liveState.lifeMin > liveState.lifeMax) {
                liveState.lifeMax = liveState.lifeMin;
            }
            break;
        case 2:
            liveState.lifeMax = value;
            if (liveState.lifeMax < liveState.lifeMin) {
                liveState.lifeMin = liveState.lifeMax;
            }
            break;
        case 3:
            liveState.sizeStart = value;
            break;
        case 4:
            liveState.sizeEnd = value;
            break;
        case 5:
            liveState.colorStart.x = value;
            break;
        case 6:
            liveState.colorStart.y = value;
            break;
        case 7:
            liveState.colorStart.z = value;
            break;
        case 8:
            liveState.spreadRadians = value;
            break;
        case 9:
            liveState.speedMin = value;
            if (liveState.speedMin > liveState.speedMax) {
                liveState.speedMax = liveState.speedMin;
            }
            break;
        case 10:
            liveState.speedMax = value;
            if (liveState.speedMax < liveState.speedMin) {
                liveState.speedMin = liveState.speedMax;
            }
            break;
        case 11:
            liveState.emissionModule = static_cast<int>(std::lround(value));
            if (liveState.emissionModule < 0) {
                liveState.emissionModule = 0;
            }
            if (liveState.emissionModule > 2) {
                liveState.emissionModule = 2;
            }
            break;
        case 12:
            liveState.ringRadius = value;
            break;
        default:
            break;
    }
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void ParticleEmitterInspectorWidget::OnEnabledChanged(const bool value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetParticleEmitterCommand::Capture(*activeCtx.target));
    liveState.enabled = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void ParticleEmitterInspectorWidget::OnLocalEmissionChanged(const bool value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetParticleEmitterCommand::Capture(*activeCtx.target));
    liveState.useLocalEmission = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void ParticleEmitterInspectorWidget::OnLocalEmissionChangedStatic(void* const userData, const bool value) {
    static_cast<ParticleEmitterInspectorWidget*>(userData)->OnLocalEmissionChanged(value);
}

void ParticleEmitterInspectorWidget::OnEnabledChangedStatic(void* const userData, const bool value) {
    static_cast<ParticleEmitterInspectorWidget*>(userData)->OnEnabledChanged(value);
}

}  // namespace Spark::Editor
