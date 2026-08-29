#pragma once

#include "spark/core/Array.hpp"
#include "spark/editor/commands/SetPointLightCommand.hpp"
#include "spark/editor/inspector/IInspectorWidget.hpp"
#include "spark/editor/inspector/InspectorEditTracker.hpp"
#include "spark/editor/inspector/InspectorUiBuilder.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark::Editor {

class PointLightInspectorWidget final : public IInspectorWidget {
public:
    [[nodiscard]] ComponentKind GetComponentKind() const noexcept override { return ComponentKind::PointLight; }
    [[nodiscard]] bool IsRelevantFor(const GameObject* target) const noexcept override;

    void BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) override;
    void SetSectionVisible(bool visible) override;
    void SyncFromTarget(const InspectorWidgetContext& ctx) override;
    void CommitPendingEdits(const InspectorWidgetContext& ctx) override;
    void CancelPendingEdits() override;
    void PrepareFrameContext(const InspectorWidgetContext& ctx) override;
    [[nodiscard]] bool HasPendingEdits() const noexcept override { return editTracker.HasPendingEdit(); }

private:
    void OnColorChanged(int axis, float value);
    void OnIntensityChanged(float value);
    void OnRangeChanged(float value);
    void OnEnabledChanged(bool value);
    void ApplyLiveEdit();

    static void OnColorRChanged(void* userData, float value);
    static void OnColorGChanged(void* userData, float value);
    static void OnColorBChanged(void* userData, float value);
    static void OnIntensitySliderChanged(void* userData, float value);
    static void OnRangeSliderChanged(void* userData, float value);
    static void OnEnabledCheckboxChanged(void* userData, bool value);

    Ui::IPanel* sectionPanel = nullptr;
    Ui::ICheckBox* enabledBox = nullptr;
    Array<InspectorSliderBinding> bindings{};
    InspectorEditTracker<PointLightState> editTracker{};
    InspectorWidgetContext activeCtx{};
    PointLightState liveState{};
    bool suppressSync = false;
    bool built = false;
};

}  // namespace Spark::Editor
