#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObjectQuery.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/core/EntityId.hpp"

namespace Spark {

class GameObject;
class GameWorld;

/**
 * Entity storage, hierarchy, deferred destroy, and query iteration (single responsibility).
 * <c>GameWorld</c> delegates here; <c>GameObject</c> lifetime is tied to the owning world.
 */
class GameWorldEntityGraph {
public:
    explicit GameWorldEntityGraph(GameWorld& owner) noexcept : owner_(owner) {}

    [[nodiscard]] GameObject* Create();

    void Destroy(GameObject* object);
    void DestroyAtEndOfFrame(GameObject* object);
    void DestroyImmediate(GameObject* object);
    void FlushPendingDestroys();

    [[nodiscard]] bool SetParent(GameObject* child, GameObject* newParent);

    [[nodiscard]] std::size_t GetCount() const noexcept { return objects.GetSize(); }

    [[nodiscard]] bool IsUpdating() const noexcept { return updatingGameObjects; }
    void SetUpdating(bool value) noexcept { updatingGameObjects = value; }

    [[nodiscard]] GameObject* FindById(std::uint64_t id) const noexcept;
    [[nodiscard]] GameObject* TryGet(EntityId id) const noexcept { return FindById(id); }
    [[nodiscard]] bool IsAlive(const GameObject* object) const noexcept;
    [[nodiscard]] GameObject* FindByName(const char* name, const GameObjectQueryFilter& filter = {}) const;
    [[nodiscard]] GameObject* FindWithTag(const char* tag, const GameObjectQueryFilter& filter = {}) const;

    void Collect(GameObjectQueryResult& out, const GameObjectQueryFilter& filter) const;
    [[nodiscard]] GameObjectQueryResult Query(const GameObjectQueryFilter& filter = {}) const;

    void DestroyAllRootObjectsImmediate();

    template<typename Fn>
    void ForEach(Fn&& fn) {
        for (std::size_t i = 0; i < objects.GetSize(); ++i) {
            fn(objects[i].Get());
        }
    }

    template<typename Fn>
    void ForEach(Fn&& fn) const {
        for (std::size_t i = 0; i < objects.GetSize(); ++i) {
            fn(objects[i].Get());
        }
    }

    template<typename Fn>
    void ForEach(Fn&& fn, const GameObjectQueryFilter& filter) {
        ForEach([&](GameObject* object) {
            if (GameObjectPassesQueryFilter(object, filter)) {
                fn(object);
            }
        });
    }

    template<typename Fn>
    void ForEach(Fn&& fn, const GameObjectQueryFilter& filter) const {
        ForEach([&](GameObject* object) {
            if (GameObjectPassesQueryFilter(object, filter)) {
                fn(object);
            }
        });
    }

    template<typename Fn>
    void ForEachActive(Fn&& fn) {
        GameObjectQueryFilter filter;
        filter.includeInactive = false;
        ForEach(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEachActive(Fn&& fn) const {
        GameObjectQueryFilter filter;
        filter.includeInactive = false;
        ForEach(Forward<Fn>(fn), filter);
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        ForEach(
                [&](GameObject* object) {
                    if (GameObjectHasAllComponents<ComponentTypes...>(object)) {
                        fn(object);
                    }
                },
                filter);
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        ForEach(
                [&](GameObject* object) {
                    if (GameObjectHasAllComponents<ComponentTypes...>(object)) {
                        fn(object);
                    }
                },
                filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        ForEachWithComponents<T>([&](GameObject* object) { fn(object, *object->GetComponent<T>()); }, filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        ForEachWithComponents<T>([&](GameObject* object) { fn(object, *object->GetComponent<T>()); }, filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstWithComponents(const GameObjectQueryFilter& filter = {}) {
        GameObject* found = nullptr;
        ForEachWithComponents<ComponentTypes...>([&](GameObject* object) {
            if (found == nullptr) {
                found = object;
            }
        }, filter);
        return found;
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstWithComponents(const GameObjectQueryFilter& filter = {}) const {
        GameObject* found = nullptr;
        ForEachWithComponents<ComponentTypes...>([&](GameObject* object) {
            if (found == nullptr) {
                found = object;
            }
        }, filter);
        return found;
    }

private:
    friend class GameWorld;
    friend class GameObject;

    void EraseStorage(GameObject* pointer);
    static void CollectSubtreePostOrder(GameObject* root, Array<GameObject*>& out);
    void EnqueueDestroy(GameObject* object);
    void DestroyImmediateInternal(GameObject* object);
    static void MarkSubtreePendingDestroy(GameObject* root);
    static bool HasPendingDestroyAncestor(const GameObject* object);

    GameWorld& owner_;
    Array<UniquePtr<GameObject>> objects;
    Array<GameObject*> pendingDestroyRoots;
    bool updatingGameObjects = false;
    std::uint64_t nextObjectId = 1;
};

}  // namespace Spark
