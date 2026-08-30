#include "spark/editor/inspector/widgets/VfxPlayerInspectorWidget.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/VfxPlayerComponent.hpp"
#include "spark/ui/spark/UiChild.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"

namespace Spark::Editor {

bool VfxPlayerInspectorWidget::IsRelevantFor(const GameObject* const target) const noexcept {
    return target != nullptr && target->HasComponent<VfxPlayerComponent>();
}

void VfxPlayerInspectorWidget::BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) {
    if (built) {
        return;
    }

    Ui::PanelDesc sectionDesc{};
    sectionDesc.id = Utf8String("inspector_vfx_player_section");
    sectionDesc.title = Utf8String("VFX Player");
    auto sectionUp = factory.CreatePanel(sectionDesc);
    sectionPanel = sectionUp.Get();

    InspectorUiBuilder::AddSectionHeader(*sectionPanel, factory, "VFX Player");

    Ui::TextFieldDesc assetDesc{};
    assetDesc.id = Utf8String("inspector_vfx_player_asset");
    assetDesc.label = Utf8String("Asset Key");
    assetDesc.text = Utf8String("");
    auto assetUp = factory.CreateTextBox(assetDesc);
    assetKeyField = assetUp.Get();
    Ui::UiVoidCallback commitCb{};
    commitCb.fn = OnAssetKeyCommittedStatic;
    commitCb.userData = this;
    assetKeyField->SetOnCommit(commitCb);
    AdoptUiChild(*sectionPanel, MoveTemp(assetUp));

    Ui::CheckBoxDesc playOnStartDesc{};
    playOnStartDesc.id = Utf8String("inspector_vfx_player_play_on_start");
    playOnStartDesc.label = Utf8String("Play On Start");
    playOnStartDesc.value = false;
    auto playOnStartUp = factory.CreateCheckBox(playOnStartDesc);
    playOnStartBox = playOnStartUp.Get();
    Ui::UiBoolCallback playOnStartCb{};
    playOnStartCb.fn = OnPlayOnStartChangedStatic;
    playOnStartCb.userData = this;
    playOnStartBox->SetOnChanged(playOnStartCb);
    AdoptUiChild(*sectionPanel, MoveTemp(playOnStartUp));

    Ui::CheckBoxDesc playOnceDesc{};
    playOnceDesc.id = Utf8String("inspector_vfx_player_play_on_start_once");
    playOnceDesc.label = Utf8String("Play On Start Once");
    playOnceDesc.value = false;
    auto playOnceUp = factory.CreateCheckBox(playOnceDesc);
    playOnStartOnceBox = playOnceUp.Get();
    Ui::UiBoolCallback playOnceCb{};
    playOnceCb.fn = OnPlayOnStartOnceChangedStatic;
    playOnceCb.userData = this;
    playOnStartOnceBox->SetOnChanged(playOnceCb);
    AdoptUiChild(*sectionPanel, MoveTemp(playOnceUp));

    Ui::ButtonDesc playDesc{};
    playDesc.id = Utf8String("inspector_vfx_player_play");
    playDesc.label = Utf8String("Play");
    auto playUp = factory.CreateButton(playDesc);
    Ui::UiVoidCallback playCb{};
    playCb.fn = OnPlayClickedStatic;
    playCb.userData = this;
    playUp->SetOnClick(playCb);
    AdoptUiChild(*sectionPanel, MoveTemp(playUp));

    Ui::ButtonDesc playOnceBtnDesc{};
    playOnceBtnDesc.id = Utf8String("inspector_vfx_player_play_once");
    playOnceBtnDesc.label = Utf8String("Play Once");
    auto playOnceBtnUp = factory.CreateButton(playOnceBtnDesc);
    Ui::UiVoidCallback playOnceBtnCb{};
    playOnceBtnCb.fn = OnPlayOnceClickedStatic;
    playOnceBtnCb.userData = this;
    playOnceBtnUp->SetOnClick(playOnceBtnCb);
    AdoptUiChild(*sectionPanel, MoveTemp(playOnceBtnUp));

    Ui::ButtonDesc stopDesc{};
    stopDesc.id = Utf8String("inspector_vfx_player_stop");
    stopDesc.label = Utf8String("Stop");
    auto stopUp = factory.CreateButton(stopDesc);
    Ui::UiVoidCallback stopCb{};
    stopCb.fn = OnStopClickedStatic;
    stopCb.userData = this;
    stopUp->SetOnClick(stopCb);
    AdoptUiChild(*sectionPanel, MoveTemp(stopUp));

    AdoptUiChild(parent, MoveTemp(sectionUp));
    built = true;
}

void VfxPlayerInspectorWidget::SetSectionVisible(const bool visible) {
    if (sectionPanel != nullptr) {
        sectionPanel->SetVisible(visible);
    }
}

void VfxPlayerInspectorWidget::SyncFromTarget(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
    if (!built || suppressSync || editTracker.HasPendingEdit()) {
        return;
    }
    if (!IsRelevantFor(ctx.target)) {
        return;
    }
    liveState = SetVfxPlayerCommand::Capture(*ctx.target);
    if (assetKeyField != nullptr) {
        assetKeyField->SetText(liveState.assetKey);
    }
    if (playOnStartBox != nullptr) {
        playOnStartBox->SetValue(liveState.playOnStart);
    }
    if (playOnStartOnceBox != nullptr) {
        playOnStartOnceBox->SetValue(liveState.playOnStartOnce);
    }
}

void VfxPlayerInspectorWidget::CommitPendingEdits(const InspectorWidgetContext& ctx) {
    if (!IsRelevantFor(ctx.target) || !ctx.CanEdit()) {
        editTracker.Cancel();
        return;
    }
    editTracker.TryCommit(liveState, [&](const VfxPlayerState& before, const VfxPlayerState& after) {
        if (SetVfxPlayerCommand::NearlyEqual(before, after) || ctx.commandStack == nullptr) {
            return;
        }
        auto command = MakeUnique<SetVfxPlayerCommand>(*ctx.target, before, after);
        ctx.commandStack->Record(UniquePtr<IEditorCommand>(command.Release()));
    });
}

void VfxPlayerInspectorWidget::CancelPendingEdits() {
    editTracker.Cancel();
}

void VfxPlayerInspectorWidget::PrepareFrameContext(const InspectorWidgetContext& ctx) {
    activeCtx = ctx;
}

void VfxPlayerInspectorWidget::ApplyLiveEdit() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (VfxPlayerComponent* player = activeCtx.target->GetComponent<VfxPlayerComponent>()) {
        suppressSync = true;
        player->SetVfxAssetKey(liveState.assetKey.CStr());
        player->SetPlayOnStart(liveState.playOnStart);
        player->SetPlayOnStartOnce(liveState.playOnStartOnce);
        suppressSync = false;
    }
}

void VfxPlayerInspectorWidget::OnAssetKeyCommitted() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target) || assetKeyField == nullptr) {
        return;
    }
    editTracker.BeginEdit(SetVfxPlayerCommand::Capture(*activeCtx.target));
    liveState.assetKey = Utf8String(assetKeyField->GetText().CStr());
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void VfxPlayerInspectorWidget::OnPlayOnStartChanged(const bool value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetVfxPlayerCommand::Capture(*activeCtx.target));
    liveState.playOnStart = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void VfxPlayerInspectorWidget::OnPlayOnStartOnceChanged(const bool value) {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    editTracker.BeginEdit(SetVfxPlayerCommand::Capture(*activeCtx.target));
    liveState.playOnStartOnce = value;
    editTracker.MarkChanged();
    ApplyLiveEdit();
}

void VfxPlayerInspectorWidget::OnPlayClicked() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (VfxPlayerComponent* player = activeCtx.target->GetComponent<VfxPlayerComponent>()) {
        player->Play(*activeCtx.target);
    }
}

void VfxPlayerInspectorWidget::OnPlayOnceClicked() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (VfxPlayerComponent* player = activeCtx.target->GetComponent<VfxPlayerComponent>()) {
        player->PlayOnce(*activeCtx.target);
    }
}

void VfxPlayerInspectorWidget::OnStopClicked() {
    if (!activeCtx.CanApplyLiveEdit() || !IsRelevantFor(activeCtx.target)) {
        return;
    }
    if (VfxPlayerComponent* player = activeCtx.target->GetComponent<VfxPlayerComponent>()) {
        player->Stop(*activeCtx.target);
    }
}

void VfxPlayerInspectorWidget::OnAssetKeyCommittedStatic(void* const userData) {
    static_cast<VfxPlayerInspectorWidget*>(userData)->OnAssetKeyCommitted();
}

void VfxPlayerInspectorWidget::OnPlayOnStartChangedStatic(void* const userData, const bool value) {
    static_cast<VfxPlayerInspectorWidget*>(userData)->OnPlayOnStartChanged(value);
}

void VfxPlayerInspectorWidget::OnPlayOnStartOnceChangedStatic(void* const userData, const bool value) {
    static_cast<VfxPlayerInspectorWidget*>(userData)->OnPlayOnStartOnceChanged(value);
}

void VfxPlayerInspectorWidget::OnPlayClickedStatic(void* const userData) {
    static_cast<VfxPlayerInspectorWidget*>(userData)->OnPlayClicked();
}

void VfxPlayerInspectorWidget::OnPlayOnceClickedStatic(void* const userData) {
    static_cast<VfxPlayerInspectorWidget*>(userData)->OnPlayOnceClicked();
}

void VfxPlayerInspectorWidget::OnStopClickedStatic(void* const userData) {
    static_cast<VfxPlayerInspectorWidget*>(userData)->OnStopClicked();
}

}  // namespace Spark::Editor
