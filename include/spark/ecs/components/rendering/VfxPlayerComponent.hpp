#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"

namespace Spark {

class GameObject;
class ParticleEmitterComponent;

enum class VfxPlaybackState : std::uint8_t {
    Stopped = 0,
    Playing,
    PlayingOnce,
};

/**
 * Plays a <c>VfxAsset</c> (single emitter, composite layers, or optional prefab child).
 * One-shots finish when all particle layers are idle; pooled instances recycle via <c>VfxSubsystem</c>.
 */
class VfxPlayerComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::VfxPlayer;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    void SetVfxAssetKey(const char* key);
    [[nodiscard]] const Utf8String& GetVfxAssetKey() const noexcept { return vfxAssetKey; }

    void SetPlayOnStart(bool enabled) noexcept { playOnStart = enabled; }
    [[nodiscard]] bool GetPlayOnStart() const noexcept { return playOnStart; }

    void SetPlayOnStartOnce(bool enabled) noexcept { playOnStartOnce = enabled; }
    [[nodiscard]] bool GetPlayOnStartOnce() const noexcept { return playOnStartOnce; }

    void SetPooledPlayback(bool pooled) noexcept { pooledPlayback = pooled; }
    [[nodiscard]] bool IsPooledPlayback() const noexcept { return pooledPlayback; }

    void Play(GameObject& owner);
    void PlayOnce(GameObject& owner);
    void Stop(GameObject& owner) noexcept;

    [[nodiscard]] bool IsPlaying() const noexcept {
        return state == VfxPlaybackState::Playing || state == VfxPlaybackState::PlayingOnce;
    }
    [[nodiscard]] VfxPlaybackState GetPlaybackState() const noexcept { return state; }

    /** True when a pooled one-shot has fully stopped and can be returned to the pool. */
    [[nodiscard]] bool IsPooledPlaybackComplete() const noexcept {
        return pooledPlayback && state == VfxPlaybackState::Stopped;
    }

private:
    struct CompositePhaseRuntime {
        GameObject* emitterObject = nullptr;
        bool triggered = false;
        float stopEmissionAtAge = -1.0F;
    };

    [[nodiscard]] ParticleEmitterComponent* EnsureEmitter(GameObject& owner);
    bool TryActivateSingle(GameObject& owner, const VfxAsset& asset, VfxPlaybackState targetState);
    bool TryActivateComposite(GameObject& owner, const VfxAsset& asset, VfxPlaybackState targetState);
    void TriggerCompositePhase(GameObject& owner, std::size_t phaseIndex);
    void UpdateCompositePlayback(const FrameTiming& timing, GameObject& owner);
    [[nodiscard]] bool AreCompositeParticlesIdle() const;
    void ClearCompositeChildren(GameObject& owner) noexcept;
    bool TrySpawnPrefab(GameObject& owner, const char* prefabPath);
    void ClearPrefabChildren(GameObject& owner) noexcept;

    Utf8String vfxAssetKey{};
    VfxAsset activeAsset{};
    Array<CompositePhaseRuntime> compositePhases{};
    Array<GameObject*> prefabRoots{};
    VfxPlaybackState state = VfxPlaybackState::Stopped;
    bool playOnStart = false;
    bool playOnStartOnce = false;
    bool pooledPlayback = false;
    bool startedThisFrame = false;
    bool usingComposite = false;
    float playbackAge = 0.0F;
};

}  // namespace Spark
