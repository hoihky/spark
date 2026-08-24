#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"

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

void SkinnedMeshComponent::SetSkeleton(SharedPtr<Skeleton> sk) {
    skeleton = MoveTemp(sk);
    GameObject* o = GetOwner();
    if (o != nullptr) {
        o->EmitSignal(SignalId::MeshDirty, SignalPayload{}, this);
    }
}

}  // namespace Spark
