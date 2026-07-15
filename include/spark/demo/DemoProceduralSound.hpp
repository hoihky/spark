#pragma once

#include "spark/audio/SoundClip.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/engine/IEngineContext.hpp"

namespace Spark {

/** Plays a decoded clip if the engine exposes a running <c>SoundEngine</c>. */
inline void DemoPlayProceduralClip(IEngineContext& ctx, const SharedPtr<SoundClip>& clip, float volume = 1.0F) noexcept {
    SoundEngine* audio = ctx.TryGetSoundEngine();
    if (audio == nullptr || !audio->IsRunning() || !clip) {
        return;
    }
    audio->GetMixer().PlayOneShot(clip, volume);
}

/** Short procedural tones cached for demos (no WAV assets). */
namespace DemoSfx {

[[nodiscard]] inline SharedPtr<SoundClip>& ClipMenuLaunch() noexcept {
    static SharedPtr<SoundClip> c = SoundClip::CreateToneBlip(560.0F, 0.045F, 0.22F);
    return c;
}

[[nodiscard]] inline SharedPtr<SoundClip>& ClipSteeringMode() noexcept {
    static SharedPtr<SoundClip> c = SoundClip::CreateToneBlip(420.0F, 0.055F, 0.2F);
    return c;
}

[[nodiscard]] inline SharedPtr<SoundClip>& ClipPhysicsThrow() noexcept {
    static SharedPtr<SoundClip> c = SoundClip::CreateToneBlip(300.0F, 0.048F, 0.26F);
    return c;
}

[[nodiscard]] inline SharedPtr<SoundClip>& ClipPhysicsBounce() noexcept {
    static SharedPtr<SoundClip> c = SoundClip::CreateToneBlip(680.0F, 0.028F, 0.19F);
    return c;
}

[[nodiscard]] inline SharedPtr<SoundClip>& ClipTetrisLock() noexcept {
    static SharedPtr<SoundClip> c = SoundClip::CreateToneBlip(200.0F, 0.038F, 0.17F);
    return c;
}

[[nodiscard]] inline SharedPtr<SoundClip>& ClipInvadersShoot() noexcept {
    static SharedPtr<SoundClip> c = SoundClip::CreateToneBlip(920.0F, 0.028F, 0.16F);
    return c;
}

[[nodiscard]] inline SharedPtr<SoundClip>& ClipInvadersHit() noexcept {
    static SharedPtr<SoundClip> c = SoundClip::CreateToneBlip(360.0F, 0.065F, 0.23F);
    return c;
}

/** Collectible gem pickup (2D / 3D maze demos). */
[[nodiscard]] inline SharedPtr<SoundClip>& ClipGemCollect() noexcept {
    static SharedPtr<SoundClip> c = SoundClip::CreateToneBlip(880.0F, 0.05F, 0.2F);
    return c;
}

/** Tetris piece rotation (successful kick or center rotation). */
[[nodiscard]] inline SharedPtr<SoundClip>& ClipTetrisRotate() noexcept {
    static SharedPtr<SoundClip> c = SoundClip::CreateToneBlip(660.0F, 0.032F, 0.16F);
    return c;
}

}  // namespace DemoSfx

}  // namespace Spark
