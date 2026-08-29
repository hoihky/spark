#include "spark/editor/commands/SetGameObjectNameCommand.hpp"

#include "spark/ecs/GameObject.hpp"

namespace Spark::Editor {

SetGameObjectNameCommand::SetGameObjectNameCommand(
        GameObject& target,
        Utf8String before,
        Utf8String after)
    : target(&target), before(MoveTemp(before)), after(MoveTemp(after)) {}

void SetGameObjectNameCommand::Undo() {
    if (target != nullptr) {
        target->GetName() = before;
    }
}

void SetGameObjectNameCommand::Redo() {
    if (target != nullptr) {
        target->GetName() = after;
    }
}

Utf8String SetGameObjectNameCommand::GetDescription() const {
    if (target != nullptr) {
        Utf8String desc = Utf8String("Rename ");
        desc.AppendUtf8(before.CStr());
        desc.AppendUtf8(" -> ");
        desc.AppendUtf8(after.CStr());
        return desc;
    }
    return Utf8String("Rename object");
}

}  // namespace Spark::Editor
