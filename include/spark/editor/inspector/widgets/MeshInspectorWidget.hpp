#pragma once

#include "spark/core/Array.hpp"
#include "spark/editor/inspector/IInspectorWidget.hpp"
#include "spark/editor/inspector/InspectorEditTracker.hpp"
#include "spark/editor/inspector/InspectorUiBuilder.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark::Editor {

class MeshInspectorWidget final : public IInspectorWidget {
public:
    [[nodiscard]] ComponentKind GetComponentKind() const noexcept override { return ComponentKind::Mesh; }
    [[nodiscard]] bool IsRelevantFor(const GameObject* target) const noexcept override;

    void BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) override;
    void SetSectionVisible(bool visible) override;
    void SyncFromTarget(const InspectorWidgetContext& ctx) override;
    void CommitPendingEdits(const InspectorWidgetContext& ctx) override;
    void CancelPendingEdits() override;
    void PrepareFrameContext(const InspectorWidgetContext& ctx) override;
    [[nodiscard]] bool HasPendingEdits() const noexcept override { return editTracker.HasPendingEdit(); }

private:
    void OnAlbedoChanged(int axis, float value);
    void ApplyLiveEdit();

    static void OnAlbedoRChanged(void* userData, float value);
    static void OnAlbedoGChanged(void* userData, float value);
    static void OnAlbedoBChanged(void* userData, float value);

    Ui::IPanel* sectionPanel = nullptr;
    Ui::ILabel* infoLabel = nullptr;
    Array<InspectorSliderBinding> bindings{};
    InspectorEditTracker<Vector3> editTracker{};
    InspectorWidgetContext activeCtx{};
    Vector3 liveAlbedo{1.0F, 1.0F, 1.0F};
    bool suppressSync = false;
    bool built = false;
};

}  // namespace Spark::Editor
