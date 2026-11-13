#include "spark/scene/core/GameWorldEntityGraph.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

GameObject* GameWorldEntityGraph::Create() {
    const std::uint64_t oid = nextObjectId++;
    objects.PushBack(UniquePtr<GameObject>(new GameObject(owner_, oid)));
    return objects.GetLast().Get();
}

void GameWorldEntityGraph::MarkSubtreePendingDestroy(GameObject* root) {
    if (root == nullptr || root->pendingDestroy) {
        return;
    }
    Array<GameObject*> order;
    CollectSubtreePostOrder(root, order);
    for (std::size_t i = 0; i < order.GetSize(); ++i) {
        order[i]->pendingDestroy = true;
    }
}

bool GameWorldEntityGraph::HasPendingDestroyAncestor(const GameObject* object) {
    for (const GameObject* p = object != nullptr ? object->parent : nullptr; p != nullptr; p = p->parent) {
        if (p->pendingDestroy) {
            return true;
        }
    }
    return false;
}

void GameWorldEntityGraph::CollectSubtreePostOrder(GameObject* root, Array<GameObject*>& out) {
    if (root == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < root->GetChildren().GetSize(); ++i) {
        CollectSubtreePostOrder(root->GetChildren()[i], out);
    }
    out.PushBack(root);
}

void GameWorldEntityGraph::DestroyImmediateInternal(GameObject* object) {
    if (object == nullptr || object->world != &owner_) {
        return;
    }
    Array<GameObject*> order;
    CollectSubtreePostOrder(object, order);
    for (std::size_t i = 0; i < order.GetSize(); ++i) {
        GameObject* n = order[i];
        n->pendingDestroy = false;
        if (n->parent != nullptr) {
            n->parent->InternalRemoveChild(n);
        }
        n->parent = nullptr;
        n->children.Clear();
        EraseStorage(n);
    }
}

void GameWorldEntityGraph::DestroyImmediate(GameObject* object) {
    DestroyImmediateInternal(object);
}

void GameWorldEntityGraph::EnqueueDestroy(GameObject* object) {
    if (object == nullptr || object->world != &owner_) {
        return;
    }
    if (object->pendingDestroy || HasPendingDestroyAncestor(object)) {
        return;
    }

    for (std::size_t i = pendingDestroyRoots.GetSize(); i > 0; --i) {
        GameObject* queuedRoot = pendingDestroyRoots[i - 1];
        if (queuedRoot == nullptr) {
            continue;
        }
        for (const GameObject* p = object; p != nullptr; p = p->parent) {
            if (p == queuedRoot) {
                pendingDestroyRoots.RemoveAt(i - 1);
                break;
            }
        }
    }

    MarkSubtreePendingDestroy(object);
    pendingDestroyRoots.PushBack(object);
}

void GameWorldEntityGraph::DestroyAtEndOfFrame(GameObject* object) {
    EnqueueDestroy(object);
    if (!updatingGameObjects) {
        FlushPendingDestroys();
    }
}

void GameWorldEntityGraph::FlushPendingDestroys() {
    const Array<GameObject*> roots = pendingDestroyRoots;
    pendingDestroyRoots.Clear();
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        DestroyImmediateInternal(roots[i]);
    }
}

void GameWorldEntityGraph::Destroy(GameObject* object) {
    if (object == nullptr || object->world != &owner_) {
        return;
    }
    if (updatingGameObjects) {
        EnqueueDestroy(object);
        return;
    }
    DestroyImmediateInternal(object);
}

void GameWorldEntityGraph::EraseStorage(GameObject* pointer) {
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        if (objects[i].Get() == pointer) {
            objects.RemoveAt(i);
            return;
        }
    }
}

bool GameWorldEntityGraph::IsAlive(const GameObject* object) const noexcept {
    if (object == nullptr || object->world != &owner_ || object->pendingDestroy) {
        return false;
    }
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        if (objects[i].Get() == object) {
            return true;
        }
    }
    return false;
}

bool GameWorldEntityGraph::SetParent(GameObject* child, GameObject* newParent) {
    if (child == nullptr || child->world != &owner_) {
        return false;
    }
    if (newParent != nullptr) {
        if (newParent->world != &owner_) {
            return false;
        }
        if (newParent == child) {
            return false;
        }
        for (const GameObject* p = newParent; p != nullptr; p = p->parent) {
            if (p == child) {
                return false;
            }
        }
    }
    if (child->parent != nullptr) {
        child->parent->InternalRemoveChild(child);
    }
    child->parent = newParent;
    if (newParent != nullptr) {
        newParent->InternalAddChild(child);
    }
    return true;
}

GameObject* GameWorldEntityGraph::FindById(const std::uint64_t id) const noexcept {
    if (id == 0) {
        return nullptr;
    }
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        GameObject* object = objects[i].Get();
        if (object != nullptr && object->GetId() == id) {
            return object;
        }
    }
    return nullptr;
}

GameObject* GameWorldEntityGraph::FindByName(const char* name, const GameObjectQueryFilter& filter) const {
    if (name == nullptr) {
        return nullptr;
    }
    GameObject* found = nullptr;
    ForEach(
            [&](GameObject* object) {
                if (found == nullptr && object->GetName() == Utf8String(name)) {
                    found = object;
                }
            },
            filter);
    return found;
}

GameObject* GameWorldEntityGraph::FindWithTag(const char* tag, const GameObjectQueryFilter& filter) const {
    if (tag == nullptr) {
        return nullptr;
    }
    GameObjectQueryFilter tagFilter = filter;
    tagFilter.tagEquals = tag;
    GameObject* found = nullptr;
    ForEach(
            [&](GameObject* object) {
                if (found == nullptr) {
                    found = object;
                }
            },
            tagFilter);
    return found;
}

void GameWorldEntityGraph::Collect(GameObjectQueryResult& out, const GameObjectQueryFilter& filter) const {
    out.Clear();
    ForEach([&](GameObject* object) { out.objects.PushBack(object); }, filter);
}

GameObjectQueryResult GameWorldEntityGraph::Query(const GameObjectQueryFilter& filter) const {
    GameObjectQueryResult result;
    Collect(result, filter);
    return result;
}

void GameWorldEntityGraph::DestroyAllRootObjectsImmediate() {
    Array<GameObject*> roots;
    roots.Reserve(objects.GetSize());
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        GameObject* o = objects[i].Get();
        if (o != nullptr && o->GetParent() == nullptr) {
            roots.PushBack(o);
        }
    }
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        DestroyImmediateInternal(roots[i]);
    }
}

}  // namespace Spark
