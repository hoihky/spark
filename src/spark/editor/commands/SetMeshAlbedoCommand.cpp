#include "spark/editor/commands/SetMeshAlbedoCommand.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/math/Constants.hpp"

#include <cmath>

namespace Spark::Editor {

namespace {

bool FloatNearlyEqual(float a, float b) noexcept {
    return std::fabs(a - b) <= Epsilon;
}

}  // namespace

SetMeshAlbedoCommand::SetMeshAlbedoCommand(
        GameObject& target,
        Vector3 before,
        Vector3 after)
    : target(&target), before(MoveTemp(before)), after(MoveTemp(after)) {}

void SetMeshAlbedoCommand::Undo() {
    if (target != nullptr) {
        if (MeshComponent* mesh = target->GetComponent<MeshComponent>()) {
            mesh->SetAlbedo(before);
        }
    }
}

void SetMeshAlbedoCommand::Redo() {
    if (target != nullptr) {
        if (MeshComponent* mesh = target->GetComponent<MeshComponent>()) {
            mesh->SetAlbedo(after);
        }
    }
}

Utf8String SetMeshAlbedoCommand::GetDescription() const {
    if (target != nullptr) {
        Utf8String desc = Utf8String("Mesh albedo ");
        desc.AppendUtf8(target->GetName().CStr());
        return desc;
    }
    return Utf8String("Mesh albedo edit");
}

bool SetMeshAlbedoCommand::NearlyEqual(const Vector3& a, const Vector3& b) noexcept {
    return FloatNearlyEqual(a.x, b.x) && FloatNearlyEqual(a.y, b.y) && FloatNearlyEqual(a.z, b.z);
}

}  // namespace Spark::Editor
