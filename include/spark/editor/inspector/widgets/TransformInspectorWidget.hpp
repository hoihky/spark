#pragma once

#include "spark/core/Array.hpp"
#include "spark/editor/inspector/IInspectorWidget.hpp"
#include "spark/editor/inspector/InspectorEditTracker.hpp"
#include "spark/editor/inspector/InspectorUiBuilder.hpp"
#include "spark/math/Transform.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark::Editor {

class TransformInspectorWidget final : public IInspectorWidget {
public:
    [[nodiscard]] ComponentKind GetComponentKind() const noexcept override { return ComponentKind::Transform; }
    [[nodiscard]] bool IsRelevantFor(const GameObject* target) const noexcept override;

    void BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) override;
    void SetSectionVisible(bool visible) override;
    void SyncFromTarget(const InspectorWidgetContext& ctx) override;
    void CommitPendingEdits(const InspectorWidgetContext& ctx) override;
    void CancelPendingEdits() override;
    void PrepareFrameContext(const InspectorWidgetContext& ctx) override;
    [[nodiscard]] bool HasPendingEdits() const noexcept override { return editTracker.HasPendingEdit(); }

private:
    void OnPositionChanged(int axis, float value);
    void OnRotationChanged(int axis, float value);
    void OnScaleChanged(int axis, float value);

    static void OnPositionXChanged(void* userData, float value);
    static void OnPositionYChanged(void* userData, float value);
    static void OnPositionZChanged(void* userData, float value);
    static void OnRotationXChanged(void* userData, float value);
    static void OnRotationYChanged(void* userData, float value);
    static void OnRotationZChanged(void* userData, float value);
    static void OnScaleXChanged(void* userData, float value);
    static void OnScaleYChanged(void* userData, float value);
    static void OnScaleZChanged(void* userData, float value);

    void ApplyLiveEdit();

    Ui::IPanel* sectionPanel = nullptr;
    Array<InspectorSliderBinding> bindings{};
    InspectorEditTracker<Transform> editTracker{};
    InspectorWidgetContext activeCtx{};
    Transform liveTransform{};
    Vector3 liveEulerDegrees{};
    bool suppressSync = false;
    bool built = false;
};

}  // namespace Spark::Editor
