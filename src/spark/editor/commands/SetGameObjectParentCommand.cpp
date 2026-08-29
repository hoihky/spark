#include "spark/editor/commands/SetGameObjectParentCommand.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark::Editor {

SetGameObjectParentCommand::SetGameObjectParentCommand(
        GameWorld& inWorld,
        GameObject& child,
        GameObject* newParent)
    : world(&inWorld),
      childId(child.GetId()),
      parentBeforeId(child.GetParent() != nullptr ? child.GetParent()->GetId() : 0),
      parentAfterId(newParent != nullptr ? newParent->GetId() : 0),
      childName(child.GetName()) {}

void SetGameObjectParentCommand::ApplyParent(const std::uint64_t parentId) {
    if (world == nullptr || childId == 0) {
        return;
    }
    GameObject* child = world->FindGameObjectById(childId);
    if (child == nullptr) {
        return;
    }
    GameObject* parent = parentId != 0 ? world->FindGameObjectById(parentId) : nullptr;
    (void)world->SetParent(child, parent);
}

void SetGameObjectParentCommand::Undo() {
    ApplyParent(parentBeforeId);
}

void SetGameObjectParentCommand::Redo() {
    ApplyParent(parentAfterId);
}

Utf8String SetGameObjectParentCommand::GetDescription() const {
    Utf8String desc = Utf8String("Reparent ");
    desc.AppendUtf8(childName.CStr());
    return desc;
}

}  // namespace Spark::Editor
