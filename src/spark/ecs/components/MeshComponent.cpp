#include "spark/ecs/components/MeshComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

MeshComponent::MeshComponent(SharedPtr<Mesh> inMesh, SceneMeshSlot inSlot, Vector3 inAlbedo)
        : mesh(MoveTemp(inMesh)), slot(inSlot), albedo(inAlbedo) {}

MeshComponent::MeshComponent(SharedPtr<Mesh> inMesh, Vector3 inAlbedo)
        : mesh(MoveTemp(inMesh)), slot(SceneMeshSlot::Custom), albedo(inAlbedo) {}

void MeshComponent::OnSignal(GameObject& /*owner*/, SignalId /*id*/, const SignalPayload& /*payload*/) {
    // Hook for TransformChanged / MeshDirty (e.g. LOD, GPU cache); draw path reads transforms each frame.
}

void MeshComponent::SetMesh(SharedPtr<Mesh> m) {
    mesh = MoveTemp(m);
    GameObject* o = GetOwner();
    if (o != nullptr) {
        o->EmitSignal(SignalId::MeshDirty, SignalPayload{}, this);
    }
}

void MeshComponent::SetAlbedo(const Vector3& c) {
    albedo = c;
    GameObject* o = GetOwner();
    if (o != nullptr) {
        o->EmitSignal(SignalId::MeshDirty, SignalPayload{}, this);
    }
}

}  // namespace Spark
