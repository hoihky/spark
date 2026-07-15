#include "spark/ecs/components/CollisionComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"

namespace Spark {

CollisionComponent::CollisionComponent(float sphereRadius, Vector3 center)
    : radius(sphereRadius), localCenter(center) {}

void CollisionComponent::OnAttach(GameObject& owner) {
    RefreshWorldBounds(owner);
}

void CollisionComponent::OnSignal(GameObject& owner, SignalId id, const SignalPayload& /*payload*/) {
    if (id == SignalId::TransformChanged || id == SignalId::CollisionBoundsDirty) {
        RefreshWorldBounds(owner);
    }
}

void CollisionComponent::SetRadius(float r) {
    radius = r;
    GameObject* o = GetOwner();
    if (o != nullptr) {
        o->EmitSignal(SignalId::CollisionBoundsDirty, SignalPayload{}, this);
    }
}

void CollisionComponent::SetLocalCenter(const Vector3& c) {
    localCenter = c;
    GameObject* o = GetOwner();
    if (o != nullptr) {
        o->EmitSignal(SignalId::CollisionBoundsDirty, SignalPayload{}, this);
    }
}

void CollisionComponent::RefreshWorldBounds(GameObject& owner) {
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector4 hp{localCenter.x, localCenter.y, localCenter.z, 1.0F};
    const Vector4 wp = wm * hp;
    worldCenter = {wp.x / wp.w, wp.y / wp.w, wp.z / wp.w};
}

}  // namespace Spark
