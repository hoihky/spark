#include "spark/scene/vfx/VfxSubsystem.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/rendering/VfxPlayerComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void VfxSubsystem::Queue(VfxPlayRequest request) {
    if (request.assetKeyOrBuiltin.IsEmpty()) {
        return;
    }
    pending.PushBack(Forward<VfxPlayRequest>(request));
}

void VfxSubsystem::Queue(const char* assetKeyOrBuiltin, const Vector3& worldPosition) {
    if (assetKeyOrBuiltin == nullptr || assetKeyOrBuiltin[0] == '\0') {
        return;
    }
    pending.PushBack(VfxPlayRequest{Utf8String(assetKeyOrBuiltin), worldPosition});
}

GameObject* VfxSubsystem::Acquire(GameWorld& world) {
    if (!pool.IsEmpty()) {
        GameObject* object = pool.GetLast();
        pool.PopBack();
        active.PushBack(object);
        object->SetActive(true);
        return object;
    }

    GameObject* object = world.CreateGameObject();
    object->GetName() = Utf8String("__vfx_pooled");
    object->AddComponent<TransformComponent>();
    object->AddComponent<ParticleEmitterComponent>();
    VfxPlayerComponent* player = object->AddComponent<VfxPlayerComponent>();
    player->SetPooledPlayback(true);
    object->SetActive(true);
    active.PushBack(object);
    return object;
}

void VfxSubsystem::Release(GameWorld& world, GameObject* object) {
    (void)world;
    if (object == nullptr) {
        return;
    }
    if (VfxPlayerComponent* player = object->GetComponent<VfxPlayerComponent>()) {
        player->Stop(*object);
        player->SetPooledPlayback(true);
    }
    object->SetActive(false);
    for (std::size_t i = 0; i < active.GetSize(); ++i) {
        if (active[i] == object) {
            active.RemoveAt(i);
            break;
        }
    }
    pool.PushBack(object);
}

void VfxSubsystem::Reset() noexcept {
    pending.Clear();
    pool.Clear();
    active.Clear();
}

void VfxSubsystem::Shutdown(GameWorld& world) noexcept {
    for (std::size_t i = 0; i < active.GetSize(); ++i) {
        if (active[i] != nullptr) {
            world.DestroyGameObject(active[i]);
        }
    }
    for (std::size_t i = 0; i < pool.GetSize(); ++i) {
        if (pool[i] != nullptr) {
            world.DestroyGameObject(pool[i]);
        }
    }
    Reset();
}

void VfxSubsystem::Process(GameWorld& world) {
    for (std::size_t i = 0; i < pending.GetSize(); ++i) {
        const VfxPlayRequest& request = pending[i];
        GameObject* object = Acquire(world);
        if (TransformComponent* tr = object->GetComponent<TransformComponent>()) {
            tr->SetTranslation(request.worldPosition);
        }
        if (VfxPlayerComponent* player = object->GetComponent<VfxPlayerComponent>()) {
            player->SetVfxAssetKey(request.assetKeyOrBuiltin.CStr());
            player->PlayOnce(*object);
        }
    }
    pending.Clear();

    for (std::size_t i = active.GetSize(); i > 0; --i) {
        GameObject* object = active[i - 1];
        if (object == nullptr) {
            continue;
        }
        const VfxPlayerComponent* player = object->GetComponent<VfxPlayerComponent>();
        if (player != nullptr && player->IsPooledPlaybackComplete()) {
            Release(world, object);
        }
    }
}

}  // namespace Spark
