#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/scene/SceneInstanceId.hpp"
#include "spark/core/Utility.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <type_traits>

namespace Spark {

class GameWorld;
class IEngineContext;

/**
 * Entity with optional parent/child hierarchy and attached GameComponents.
 * Components on the same object exchange SignalId messages via EmitSignal / OnSignal (sender excluded).
 */
class GameObject {
public:
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&&) = delete;
    GameObject& operator=(GameObject&&) = delete;

    ~GameObject();

    [[nodiscard]] GameWorld& GetWorld() const noexcept { return *world; }
    [[nodiscard]] std::uint64_t GetId() const noexcept { return id; }

    [[nodiscard]] Utf8String& GetName() noexcept { return name; }
    [[nodiscard]] const Utf8String& GetName() const noexcept { return name; }

    /** User tag for queries (see <c>GameWorld::FindGameObjectWithTag</c>). */
    [[nodiscard]] const Utf8String& GetTag() const noexcept { return tag; }
    void SetTag(const Utf8String& value) { tag = value; }
    void SetTag(const char* utf8) { tag = Utf8String(utf8 != nullptr ? utf8 : ""); }

    /**
     * Local active flag on this object. When false, <c>IsActiveInHierarchy</c> is false and simulation /
     * rendering iterators skip this subtree unless <c>GameObjectQueryFilter::includeInactive</c> is set.
     */
    [[nodiscard]] bool IsActiveSelf() const noexcept { return activeSelf; }
    void SetActive(const bool value) noexcept { activeSelf = value; }

    /** False when this object or any ancestor has <c>IsActiveSelf() == false</c>. */
    [[nodiscard]] bool IsActiveInHierarchy() const noexcept;

    [[nodiscard]] GameObject* GetParent() const noexcept { return parent; }
    [[nodiscard]] const Array<GameObject*>& GetChildren() const noexcept { return children; }

    /** Loaded-scene tag for additive multi-scene (see SceneManager). 0 = untagged. */
    [[nodiscard]] SceneInstanceId GetSceneInstanceId() const noexcept { return sceneInstanceId; }
    void SetSceneInstanceId(const SceneInstanceId id) noexcept { sceneInstanceId = id; }

    bool SetParent(GameObject* newParent);

    /** World matrix from parent chain × TransformComponent local (Identity if no transform). */
    [[nodiscard]] Matrix4 GetWorldMatrix() const;

    void EmitSignal(SignalId id, const SignalPayload& payload, GameComponent* sender = nullptr);

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of_v<GameComponent, T>);
        auto ptr = MakeUnique<T>(Forward<Args>(args)...);
        T* raw = ptr.Get();
        raw->InternalSetOwner(this);
        // Array stores UniquePtr<GameComponent>; UniquePtr<Derived> is not convertible — release + wrap.
        components.PushBack(UniquePtr<GameComponent>(static_cast<GameComponent*>(ptr.Release())));
        raw->OnAttach(*this);
        return raw;
    }

    template<typename T>
    [[nodiscard]] T* GetComponent() {
        for (std::size_t i = 0; i < components.GetSize(); ++i) {
            GameComponent* c = components[i].Get();
            if (c != nullptr && c->Kind() == T::TypeKind) {
                return static_cast<T*>(c);
            }
        }
        return nullptr;
    }

    template<typename T>
    [[nodiscard]] const T* GetComponent() const {
        for (std::size_t i = 0; i < components.GetSize(); ++i) {
            const GameComponent* c = components[i].Get();
            if (c != nullptr && c->Kind() == T::TypeKind) {
                return static_cast<const T*>(c);
            }
        }
        return nullptr;
    }

    template<typename T>
    [[nodiscard]] bool HasComponent() const noexcept {
        return GetComponent<T>() != nullptr;
    }

    /** First component matching kind, or nullptr (used by language bindings). */
    [[nodiscard]] GameComponent* TryGetComponentByKind(ComponentKind kind) noexcept;
    [[nodiscard]] const GameComponent* TryGetComponentByKind(ComponentKind kind) const noexcept;

    [[nodiscard]] std::size_t GetComponentCount() const noexcept { return components.GetSize(); }

    void UpdateComponents(const FrameTiming& timing, IEngineContext& context);

private:
    friend class GameWorld;

    explicit GameObject(GameWorld& w, std::uint64_t objectId);

    void InternalRemoveChild(GameObject* child);
    void InternalAddChild(GameObject* child);

    GameWorld* world = nullptr;
    std::uint64_t id = 0;
    SceneInstanceId sceneInstanceId = kInvalidSceneInstanceId;
    Utf8String name;
    Utf8String tag;
    bool activeSelf = true;
    GameObject* parent = nullptr;
    Array<GameObject*> children;
    Array<UniquePtr<GameComponent>> components;
};

}  // namespace Spark
