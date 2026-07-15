#include "spark/ecs/GameObject.hpp"

#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>

namespace Spark {

GameObject::GameObject(GameWorld& w, std::uint64_t objectId) : world(&w), id(objectId) {}

GameObject::~GameObject() {
    for (std::size_t i = 0; i < components.GetSize(); ++i) {
        UniquePtr<GameComponent>& up = components[i];
        if (up) {
            up->OnDetach(*this);
            up.Reset();
        }
    }
    components.Clear();
}

bool GameObject::SetParent(GameObject* newParent) {
    return world->SetParent(this, newParent);
}

Matrix4 GameObject::GetWorldMatrix() const {
    const TransformComponent* tc = GetComponent<TransformComponent>();
    const Matrix4 local = tc != nullptr ? tc->GetLocalTransform().ToMatrix4() : Matrix4::Identity;
    if (parent == nullptr) {
        return local;
    }
    return parent->GetWorldMatrix() * local;
}

void GameObject::EmitSignal(SignalId id, const SignalPayload& payload, GameComponent* sender) {
    for (std::size_t i = 0; i < components.GetSize(); ++i) {
        GameComponent* c = components[i].Get();
        if (c == nullptr || c == sender) {
            continue;
        }
        c->OnSignal(*this, id, payload);
    }
}

void GameObject::UpdateComponents(const FrameTiming& timing, IEngineContext& context) {
    const std::size_t n = components.GetSize();
    if (n == 0) {
        return;
    }
    if (n == 1) {
        if (GameComponent* c = components[0].Get()) {
            c->OnUpdate(timing, *this, context);
        }
        return;
    }

    Array<std::size_t> order;
    order.Reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        order.PushBack(i);
    }
    std::stable_sort(order.GetData(), order.GetData() + order.GetSize(), [this](const std::size_t a, const std::size_t b) {
        const GameComponent* ca = components[a].Get();
        const GameComponent* cb = components[b].Get();
        const int pa = ca != nullptr ? ca->UpdatePriority() : 0;
        const int pb = cb != nullptr ? cb->UpdatePriority() : 0;
        return pa < pb;
    });
    for (std::size_t oi = 0; oi < order.GetSize(); ++oi) {
        GameComponent* c = components[order[oi]].Get();
        if (c != nullptr) {
            c->OnUpdate(timing, *this, context);
        }
    }
}

GameComponent* GameObject::TryGetComponentByKind(ComponentKind kind) noexcept {
    for (std::size_t i = 0; i < components.GetSize(); ++i) {
        GameComponent* c = components[i].Get();
        if (c != nullptr && c->Kind() == kind) {
            return c;
        }
    }
    return nullptr;
}

const GameComponent* GameObject::TryGetComponentByKind(ComponentKind kind) const noexcept {
    for (std::size_t i = 0; i < components.GetSize(); ++i) {
        const GameComponent* c = components[i].Get();
        if (c != nullptr && c->Kind() == kind) {
            return c;
        }
    }
    return nullptr;
}

void GameObject::InternalRemoveChild(GameObject* child) {
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] == child) {
            children.RemoveAt(i);
            return;
        }
    }
}

void GameObject::InternalAddChild(GameObject* child) {
    children.PushBack(child);
}

}  // namespace Spark
