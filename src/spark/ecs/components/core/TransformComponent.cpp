#include "spark/ecs/components/core/TransformComponent.hpp"

#include "spark/ecs/GameObject.hpp"

namespace Spark {

void TransformComponent::SetLocalTransform(const Transform& t) {
    local = t;
    NotifyTransformChanged();
}

void TransformComponent::SetTranslation(const Vector3& v) {
    local.translation = v;
    NotifyTransformChanged();
}

void TransformComponent::SetRotation(const Quaternion& q) {
    local.rotation = q;
    NotifyTransformChanged();
}

void TransformComponent::SetScale(const Vector3& s) {
    local.scale = s;
    NotifyTransformChanged();
}

void TransformComponent::SetUniformScale(float s) {
    local.scale = {s, s, s};
    NotifyTransformChanged();
}

void TransformComponent::NotifyTransformChanged() {
    GameObject* o = GetOwner();
    if (o != nullptr) {
        o->EmitSignal(SignalId::TransformChanged, SignalPayload{}, this);
    }
}

}  // namespace Spark
