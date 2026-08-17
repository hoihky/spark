#include "spark/scene/GameWorld.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/GameWorldAssetLoader.hpp"
#include "spark/text/Font.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/core/Utility.hpp"

namespace Spark {

GameWorld::GameWorld() {
    assetLoader.Start();
}

GameWorld::~GameWorld() {
    assetLoader.Shutdown();
    Array<GameObject*> roots;
    roots.Reserve(objects.GetSize());
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        GameObject* o = objects[i].Get();
        if (o != nullptr && o->GetParent() == nullptr) {
            roots.PushBack(o);
        }
    }
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        DestroyGameObject(roots[i]);
    }
}

GameObject* GameWorld::CreateGameObject() {
    const std::uint64_t oid = nextObjectId++;
    objects.PushBack(UniquePtr<GameObject>(new GameObject(*this, oid)));
    return objects.GetLast().Get();
}

void GameWorld::CollectSubtreePostOrder(GameObject* root, Array<GameObject*>& out) {
    if (root == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < root->GetChildren().GetSize(); ++i) {
        CollectSubtreePostOrder(root->GetChildren()[i], out);
    }
    out.PushBack(root);
}

void GameWorld::DestroyGameObject(GameObject* object) {
    if (object == nullptr || object->world != this) {
        return;
    }
    Array<GameObject*> order;
    CollectSubtreePostOrder(object, order);
    for (std::size_t i = 0; i < order.GetSize(); ++i) {
        GameObject* n = order[i];
        if (n->parent != nullptr) {
            n->parent->InternalRemoveChild(n);
        }
        n->parent = nullptr;
        n->children.Clear();
        EraseGameObjectStorage(n);
    }
}

void GameWorld::EraseGameObjectStorage(GameObject* pointer) {
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        if (objects[i].Get() == pointer) {
            objects.RemoveAt(i);
            return;
        }
    }
}

bool GameWorld::SetParent(GameObject* child, GameObject* newParent) {
    if (child == nullptr || child->world != this) {
        return false;
    }
    if (newParent != nullptr) {
        if (newParent->world != this) {
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

void GameWorld::UpdateGameObjects(const FrameTiming& timing, IEngineContext& context) {
    assetLoader.Pump(*this);
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        GameObject* o = objects[i].Get();
        if (o != nullptr) {
            o->UpdateComponents(timing, context);
        }
    }
}

bool GameWorld::AwaitGltf(const char* path, GltfAsset& out) {
    out = GltfAsset{};
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (TryGetCachedGltf(path, out)) {
        return static_cast<bool>(out.mesh);
    }
    RequestGltf(path);
    for (int attempt = 0; attempt < 200000; ++attempt) {
        PumpAssets();
        if (TryGetCachedGltf(path, out) && out.mesh) {
            return true;
        }
        if (GetAssetLoadState(path, AssetLoadJobKind::Gltf) == AssetLoadState::Failed) {
            return false;
        }
    }
    return false;
}

bool GameWorld::AwaitSkinnedGltf(const char* path, SkinnedGltfAsset& out) {
    out = SkinnedGltfAsset{};
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (TryGetCachedSkinnedGltf(path, out)) {
        return static_cast<bool>(out.mesh && out.skeleton);
    }
    RequestSkinnedGltf(path);
    for (int attempt = 0; attempt < 200000; ++attempt) {
        PumpAssets();
        if (TryGetCachedSkinnedGltf(path, out) && out.mesh && out.skeleton) {
            return true;
        }
        if (GetAssetLoadState(path, AssetLoadJobKind::SkinnedGltf) == AssetLoadState::Failed) {
            return false;
        }
    }
    return false;
}

void GameWorld::SetUiFont(SharedPtr<Font> font) {
    uiFont = MoveTemp(font);
}

void GameWorld::SetUiBoldFont(SharedPtr<Font> font) {
    uiBoldFont = MoveTemp(font);
}

GameObject* GameWorld::FindGameObjectById(const std::uint64_t id) const noexcept {
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

GameObject* GameWorld::FindGameObjectByName(const char* name, const GameObjectQueryFilter& filter) const {
    if (name == nullptr) {
        return nullptr;
    }
    GameObject* found = nullptr;
    ForEachGameObject(
            [&](GameObject* object) {
                if (found == nullptr && object->GetName() == Utf8String(name)) {
                    found = object;
                }
            },
            filter);
    return found;
}

GameObject* GameWorld::FindGameObjectWithTag(const char* tag, const GameObjectQueryFilter& filter) const {
    if (tag == nullptr) {
        return nullptr;
    }
    GameObjectQueryFilter tagFilter = filter;
    tagFilter.tagEquals = tag;
    GameObject* found = nullptr;
    ForEachGameObject(
            [&](GameObject* object) {
                if (found == nullptr) {
                    found = object;
                }
            },
            tagFilter);
    return found;
}

void GameWorld::CollectGameObjects(GameObjectQueryResult& out, const GameObjectQueryFilter& filter) const {
    out.Clear();
    ForEachGameObject([&](GameObject* object) { out.objects.PushBack(object); }, filter);
}

GameObjectQueryResult GameWorld::QueryGameObjects(const GameObjectQueryFilter& filter) const {
    GameObjectQueryResult result;
    CollectGameObjects(result, filter);
    return result;
}

}  // namespace Spark
