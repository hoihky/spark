#pragma once

#include "spark/core/Array.hpp"
#include "spark/editor/commands/SetParticleEmitterCommand.hpp"
#include "spark/editor/inspector/IInspectorWidget.hpp"
#include "spark/editor/inspector/InspectorEditTracker.hpp"
#include "spark/editor/inspector/InspectorUiBuilder.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark::Editor {

class ParticleEmitterInspectorWidget final : public IInspectorWidget {
public:
    [[nodiscard]] ComponentKind GetComponentKind() const noexcept override { return ComponentKind::ParticleEmitter; }
    [[nodiscard]] bool IsRelevantFor(const GameObject* target) const noexcept override;

    void BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) override;
    void SetSectionVisible(bool visible) override;
    void SyncFromTarget(const InspectorWidgetContext& ctx) override;
    void CommitPendingEdits(const InspectorWidgetContext& ctx) override;
    void CancelPendingEdits() override;
    void PrepareFrameContext(const InspectorWidgetContext& ctx) override;
    [[nodiscard]] bool HasPendingEdits() const noexcept override { return editTracker.HasPendingEdit(); }

private:
    void ApplyLiveEdit();
    void OnSliderChanged(int index, float value);
    void OnEnabledChanged(bool value);
    void OnLocalEmissionChanged(bool value);

    static void OnEnabledChangedStatic(void* userData, bool value);
    static void OnLocalEmissionChangedStatic(void* userData, bool value);

    Ui::IPanel* sectionPanel = nullptr;
    Ui::ICheckBox* enabledBox = nullptr;
    Ui::ICheckBox* localEmissionBox = nullptr;
    Array<InspectorSliderBinding> bindings{};
    InspectorEditTracker<ParticleEmitterState> editTracker{};
    InspectorWidgetContext activeCtx{};
    ParticleEmitterState liveState{};
    bool suppressSync = false;
    bool built = false;
};

}  // namespace Spark::Editor
