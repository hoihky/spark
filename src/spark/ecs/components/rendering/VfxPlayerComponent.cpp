#include "spark/ecs/components/rendering/VfxPlayerComponent.hpp"

#include "spark/config.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"
#include "spark/scene/vfx/VfxEffectDefinition.hpp"

namespace Spark {

void VfxPlayerComponent::SetVfxAssetKey(const char* key) {
    vfxAssetKey = Utf8String(key != nullptr ? key : "");
}

ParticleEmitterComponent* VfxPlayerComponent::EnsureEmitter(GameObject& owner) {
    if (ParticleEmitterComponent* pe = owner.GetComponent<ParticleEmitterComponent>()) {
        return pe;
    }
    return owner.AddComponent<ParticleEmitterComponent>();
}

bool VfxPlayerComponent::TrySpawnPrefab(GameObject& owner, const char* prefabPath) {
    if (prefabPath == nullptr || prefabPath[0] == '\0') {
        return false;
    }
    const Utf8String resolved = ScenePathResolver::ResolveReadablePath(prefabPath);
    if (resolved.IsEmpty()) {
        return false;
    }

    SceneDocument document{};
    SceneDeserializer deserializer{};
    if (!deserializer.ReadFromFile(resolved.CStr(), document)) {
        return false;
    }

    HashMap<std::uint64_t, GameObject*> idToObject{};
    SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = ScenePathResolver::AssetsRoot();
    if (!deserializer.Apply(document, owner.GetWorld(), applyCtx, &idToObject)) {
        return false;
    }

    for (std::size_t i = 0; i < document.entities.GetSize(); ++i) {
        const EntityRecord& entity = document.entities[i];
        if (entity.parentId >= 0) {
            continue;
        }
        GameObject* const* found = idToObject.Find(static_cast<std::uint64_t>(entity.id));
        if (found == nullptr || *found == nullptr) {
            continue;
        }
        (*found)->SetParent(&owner);
        prefabRoots.PushBack(*found);
    }
    return !prefabRoots.IsEmpty();
}

void VfxPlayerComponent::ClearPrefabChildren(GameObject& owner) noexcept {
    (void)owner;
    for (std::size_t i = 0; i < prefabRoots.GetSize(); ++i) {
        GameObject* root = prefabRoots[i];
        if (root != nullptr) {
            owner.GetWorld().DestroyGameObject(root);
        }
    }
    prefabRoots.Clear();
}

void VfxPlayerComponent::ClearCompositeChildren(GameObject& owner) noexcept {
    for (std::size_t i = 0; i < compositePhases.GetSize(); ++i) {
        GameObject* phaseObject = compositePhases[i].emitterObject;
        if (phaseObject != nullptr) {
            owner.GetWorld().DestroyGameObject(phaseObject);
        }
    }
    compositePhases.Clear();
    usingComposite = false;
    playbackAge = 0.0F;
}

void VfxPlayerComponent::TriggerCompositePhase(GameObject& owner, const std::size_t phaseIndex) {
    if (phaseIndex >= compositePhases.GetSize() || phaseIndex >= activeAsset.definition.emitters.GetSize()) {
        return;
    }
    CompositePhaseRuntime& runtime = compositePhases[phaseIndex];
    if (runtime.triggered || runtime.emitterObject == nullptr) {
        return;
    }
    const VfxEmitterSpec& spec = activeAsset.definition.emitters[phaseIndex];
    ParticleEmitterComponent* pe = runtime.emitterObject->GetComponent<ParticleEmitterComponent>();
    if (pe == nullptr) {
        return;
    }
    pe->ClearParticles();
    const VfxPlaybackMode mode =
            state == VfxPlaybackState::PlayingOnce ? VfxPlaybackMode::Once : VfxPlaybackMode::Continuous;
    spec.Activate(*pe, *runtime.emitterObject, mode);
    runtime.triggered = true;
    if (spec.durationSeconds > 0.0F && spec.burstCount == 0) {
        runtime.stopEmissionAtAge = playbackAge + spec.durationSeconds;
    }
    runtime.emitterObject->SetActive(true);
}

bool VfxPlayerComponent::TryActivateComposite(
        GameObject& owner,
        const VfxAsset& asset,
        const VfxPlaybackState targetState) {
    ClearCompositeChildren(owner);
    ClearPrefabChildren(owner);

    activeAsset = asset;
    usingComposite = true;
    playbackAge = 0.0F;
    compositePhases.Clear();
    compositePhases.Reserve(asset.definition.emitters.GetSize());

    GameWorld& world = owner.GetWorld();
    for (std::size_t i = 0; i < asset.definition.emitters.GetSize(); ++i) {
        GameObject* phaseObject = world.CreateGameObject();
        phaseObject->GetName() = Utf8String("__vfx_phase");
        phaseObject->SetParent(&owner);
        phaseObject->AddComponent<TransformComponent>();
        phaseObject->AddComponent<ParticleEmitterComponent>();
        phaseObject->SetActive(false);

        CompositePhaseRuntime runtime{};
        runtime.emitterObject = phaseObject;
        compositePhases.PushBack(runtime);
    }

    if (!asset.GetPrefabScenePath().IsEmpty()) {
        TrySpawnPrefab(owner, asset.GetPrefabScenePath().CStr());
    }

    state = targetState;
    startedThisFrame = true;
    for (std::size_t i = 0; i < asset.definition.emitters.GetSize(); ++i) {
        if (asset.definition.emitters[i].startTimeSeconds <= 0.0F) {
            TriggerCompositePhase(owner, i);
        }
    }
    return true;
}

bool VfxPlayerComponent::TryActivateSingle(
        GameObject& owner,
        const VfxAsset& asset,
        const VfxPlaybackState targetState) {
    ClearCompositeChildren(owner);
    ClearPrefabChildren(owner);

    activeAsset = asset;
    usingComposite = false;

    if (!asset.GetPrefabScenePath().IsEmpty()) {
        TrySpawnPrefab(owner, asset.GetPrefabScenePath().CStr());
    }

    ParticleEmitterComponent* pe = EnsureEmitter(owner);
    if (pe == nullptr) {
        return false;
    }
    pe->ClearParticles();
    const VfxPlaybackMode mode =
            targetState == VfxPlaybackState::PlayingOnce ? VfxPlaybackMode::Once : VfxPlaybackMode::Continuous;
    asset.Activate(*pe, owner, mode);
    state = targetState;
    startedThisFrame = true;
    return true;
}

void VfxPlayerComponent::Play(GameObject& owner) {
    if (vfxAssetKey.IsEmpty()) {
        return;
    }
    VfxAsset asset{};
    if (!VfxAsset::TryResolve(vfxAssetKey.CStr(), owner.GetWorld(), asset)) {
        return;
    }
    if (asset.IsComposite()) {
        if (!TryActivateComposite(owner, asset, VfxPlaybackState::Playing)) {
            return;
        }
    } else if (!TryActivateSingle(owner, asset, VfxPlaybackState::Playing)) {
        return;
    }
    if (!usingComposite) {
        if (ParticleEmitterComponent* pe = owner.GetComponent<ParticleEmitterComponent>()) {
            pe->SetEmitterEnabled(true);
        }
    }
}

void VfxPlayerComponent::PlayOnce(GameObject& owner) {
    if (vfxAssetKey.IsEmpty()) {
        return;
    }
    VfxAsset asset{};
    if (!VfxAsset::TryResolve(vfxAssetKey.CStr(), owner.GetWorld(), asset)) {
        return;
    }
    if (asset.IsComposite()) {
        if (!TryActivateComposite(owner, asset, VfxPlaybackState::PlayingOnce)) {
            return;
        }
    } else if (!TryActivateSingle(owner, asset, VfxPlaybackState::PlayingOnce)) {
        return;
    }
    if (!usingComposite) {
        if (ParticleEmitterComponent* pe = owner.GetComponent<ParticleEmitterComponent>()) {
            pe->SetEmitterEnabled(true);
        }
    }
}

void VfxPlayerComponent::Stop(GameObject& owner) noexcept {
    if (!usingComposite) {
        if (ParticleEmitterComponent* pe = owner.GetComponent<ParticleEmitterComponent>()) {
            pe->SetEmitterEnabled(false);
            pe->ClearParticles();
        }
    }
    ClearCompositeChildren(owner);
    ClearPrefabChildren(owner);
    state = VfxPlaybackState::Stopped;
    startedThisFrame = false;
    playbackAge = 0.0F;
    (void)owner;
}

bool VfxPlayerComponent::AreCompositeParticlesIdle() const {
    for (std::size_t i = 0; i < compositePhases.GetSize(); ++i) {
        const GameObject* phaseObject = compositePhases[i].emitterObject;
        if (phaseObject == nullptr) {
            continue;
        }
        const ParticleEmitterComponent* pe = phaseObject->GetComponent<ParticleEmitterComponent>();
        if (pe != nullptr && pe->GetAliveParticleCount() > 0) {
            return false;
        }
    }
    return true;
}

void VfxPlayerComponent::UpdateCompositePlayback(const FrameTiming& timing, GameObject& owner) {
    playbackAge += timing.deltaTimeSeconds;

    for (std::size_t i = 0; i < compositePhases.GetSize(); ++i) {
        CompositePhaseRuntime& runtime = compositePhases[i];
        if (!runtime.triggered && i < activeAsset.definition.emitters.GetSize()) {
            if (playbackAge >= activeAsset.definition.emitters[i].startTimeSeconds) {
                TriggerCompositePhase(owner, i);
            }
        }
        if (runtime.stopEmissionAtAge >= 0.0F && playbackAge >= runtime.stopEmissionAtAge) {
            if (ParticleEmitterComponent* pe = runtime.emitterObject->GetComponent<ParticleEmitterComponent>()) {
                pe->SetEmissionRate(0.0F);
            }
            runtime.stopEmissionAtAge = -1.0F;
        }
    }
}

void VfxPlayerComponent::OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& /*context*/) {
    if (state == VfxPlaybackState::Stopped) {
        if (playOnStartOnce) {
            playOnStartOnce = false;
            PlayOnce(owner);
        } else if (playOnStart) {
            Play(owner);
        }
        return;
    }

    if (usingComposite) {
        if (startedThisFrame) {
            startedThisFrame = false;
            return;
        }
        UpdateCompositePlayback(timing, owner);
        if (state == VfxPlaybackState::PlayingOnce) {
            bool allTriggered = true;
            for (std::size_t i = 0; i < compositePhases.GetSize(); ++i) {
                if (!compositePhases[i].triggered) {
                    allTriggered = false;
                    break;
                }
            }
            const float minAge = activeAsset.definition.GetEstimatedDurationSeconds();
            if (allTriggered && playbackAge >= minAge && AreCompositeParticlesIdle()) {
                Stop(owner);
            }
        }
        return;
    }

    ParticleEmitterComponent* pe = owner.GetComponent<ParticleEmitterComponent>();
    if (pe == nullptr) {
        Stop(owner);
        return;
    }

    if (state == VfxPlaybackState::PlayingOnce) {
        if (startedThisFrame) {
            startedThisFrame = false;
            return;
        }
        if (pe->GetAliveParticleCount() == 0) {
            Stop(owner);
        }
    }
}

}  // namespace Spark
