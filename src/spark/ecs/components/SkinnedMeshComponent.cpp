#include "spark/ecs/components/SkinnedMeshComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/SkinnedMesh.hpp"

namespace Spark {

SkinnedMeshComponent::SkinnedMeshComponent(SharedPtr<SkinnedMesh> inMesh) : mesh(MoveTemp(inMesh)) {}

void SkinnedMeshComponent::OnSignal(GameObject& /*owner*/, SignalId /*id*/, const SignalPayload& /*payload*/) {
}

void SkinnedMeshComponent::SetMesh(SharedPtr<SkinnedMesh> m) {
    mesh = MoveTemp(m);
    GameObject* o = GetOwner();
    if (o != nullptr) {
        o->EmitSignal(SignalId::MeshDirty, SignalPayload{}, this);
    }
}

}  // namespace Spark
