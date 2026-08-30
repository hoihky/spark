#pragma once

#include "spark/editor/commands/SetVfxPlayerCommand.hpp"
#include "spark/editor/inspector/IInspectorWidget.hpp"
#include "spark/editor/inspector/InspectorEditTracker.hpp"
#include "spark/editor/inspector/InspectorUiBuilder.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark::Editor {

class VfxPlayerInspectorWidget final : public IInspectorWidget {
public:
    [[nodiscard]] ComponentKind GetComponentKind() const noexcept override { return ComponentKind::VfxPlayer; }
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
    void OnAssetKeyCommitted();
    void OnPlayOnStartChanged(bool value);
    void OnPlayOnStartOnceChanged(bool value);
    void OnPlayClicked();
    void OnPlayOnceClicked();
    void OnStopClicked();

    static void OnAssetKeyCommittedStatic(void* userData);
    static void OnPlayOnStartChangedStatic(void* userData, bool value);
    static void OnPlayOnStartOnceChangedStatic(void* userData, bool value);
    static void OnPlayClickedStatic(void* userData);
    static void OnPlayOnceClickedStatic(void* userData);
    static void OnStopClickedStatic(void* userData);

    Ui::IPanel* sectionPanel = nullptr;
    Ui::ITextBox* assetKeyField = nullptr;
    Ui::ICheckBox* playOnStartBox = nullptr;
    Ui::ICheckBox* playOnStartOnceBox = nullptr;
    InspectorEditTracker<VfxPlayerState> editTracker{};
    InspectorWidgetContext activeCtx{};
    VfxPlayerState liveState{};
    bool suppressSync = false;
    bool built = false;
};

}  // namespace Spark::Editor
