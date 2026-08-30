#include "spark/editor/commands/SetVfxPlayerCommand.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/VfxPlayerComponent.hpp"

namespace Spark::Editor {

SetVfxPlayerCommand::SetVfxPlayerCommand(
        GameObject& target,
        VfxPlayerState before,
        VfxPlayerState after)
    : target(&target), before(MoveTemp(before)), after(MoveTemp(after)) {}

void SetVfxPlayerCommand::Apply(GameObject& target, const VfxPlayerState& state) {
    if (VfxPlayerComponent* player = target.GetComponent<VfxPlayerComponent>()) {
        player->SetVfxAssetKey(state.assetKey.CStr());
        player->SetPlayOnStart(state.playOnStart);
        player->SetPlayOnStartOnce(state.playOnStartOnce);
    }
}

void SetVfxPlayerCommand::Undo() {
    if (target != nullptr) {
        Apply(*target, before);
    }
}

void SetVfxPlayerCommand::Redo() {
    if (target != nullptr) {
        Apply(*target, after);
    }
}

Utf8String SetVfxPlayerCommand::GetDescription() const {
    if (target != nullptr) {
        Utf8String desc = Utf8String("VFX Player ");
        desc.AppendUtf8(target->GetName().CStr());
        return desc;
    }
    return Utf8String("VFX player edit");
}

bool SetVfxPlayerCommand::NearlyEqual(const VfxPlayerState& a, const VfxPlayerState& b) noexcept {
    return a.assetKey == b.assetKey && a.playOnStart == b.playOnStart && a.playOnStartOnce == b.playOnStartOnce;
}

VfxPlayerState SetVfxPlayerCommand::Capture(GameObject& target) {
    VfxPlayerState state{};
    if (const VfxPlayerComponent* player = target.GetComponent<VfxPlayerComponent>()) {
        state.assetKey = player->GetVfxAssetKey();
        state.playOnStart = player->GetPlayOnStart();
        state.playOnStartOnce = player->GetPlayOnStartOnce();
    }
    return state;
}

}  // namespace Spark::Editor
