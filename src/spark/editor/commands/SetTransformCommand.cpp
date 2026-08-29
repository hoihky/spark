#include "spark/editor/commands/SetTransformCommand.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Constants.hpp"

#include <cmath>

namespace Spark::Editor {

namespace {

bool NearlyEqual(float a, float b) noexcept {
    return std::fabs(a - b) <= Epsilon;
}

bool NearlyEqual(const Vector3& a, const Vector3& b) noexcept {
    return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y) && NearlyEqual(a.z, b.z);
}

bool NearlyEqual(const Quaternion& a, const Quaternion& b) noexcept {
    return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y) && NearlyEqual(a.z, b.z) && NearlyEqual(a.w, b.w);
}

}  // namespace

SetTransformCommand::SetTransformCommand(
        GameObject& target,
        Transform before,
        Transform after)
    : target(&target), before(MoveTemp(before)), after(MoveTemp(after)) {}

void SetTransformCommand::Undo() {
    if (target == nullptr) {
        return;
    }
    if (TransformComponent* transform = target->GetComponent<TransformComponent>()) {
        transform->SetLocalTransform(before);
    }
}

void SetTransformCommand::Redo() {
    if (target == nullptr) {
        return;
    }
    if (TransformComponent* transform = target->GetComponent<TransformComponent>()) {
        transform->SetLocalTransform(after);
    }
}

Utf8String SetTransformCommand::GetDescription() const {
    if (target != nullptr) {
        Utf8String desc = Utf8String("Transform ");
        desc.AppendUtf8(target->GetName().CStr());
        return desc;
    }
    return Utf8String("Transform edit");
}

bool SetTransformCommand::NearlyEqual(const Transform& a, const Transform& b) noexcept {
    return ::Spark::Editor::NearlyEqual(a.translation, b.translation) &&
           ::Spark::Editor::NearlyEqual(a.rotation, b.rotation) &&
           ::Spark::Editor::NearlyEqual(a.scale, b.scale);
}

}  // namespace Spark::Editor
