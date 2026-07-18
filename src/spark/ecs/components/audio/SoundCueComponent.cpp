#include "spark/ecs/components/audio/SoundCueComponent.hpp"

#include "spark/audio/SoundEngine.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

void SoundCueComponent::Queue(const SharedPtr<SoundClip>& clip, const float volume) {
    if (!clip) {
        return;
    }
    SoundPlayRequest r{};
    r.clip = clip;
    r.volume = volume;
    pending.PushBack(MoveTemp(r));
}

void SoundCueComponent::QueueAtWorld(
        const SharedPtr<SoundClip>& clip,
        const float volume,
        const Vector3& worldPosition,
        const float spatialBlend,
        const float minDistance,
        const float maxDistance) {
    if (!clip) {
        return;
    }
    SoundPlayRequest r{};
    r.clip = clip;
    r.volume = volume;
    r.worldPosition = worldPosition;
    r.spatialBlend = spatialBlend;
    r.minDistance = minDistance;
    r.maxDistance = maxDistance;
    pending.PushBack(MoveTemp(r));
}

void SoundCueComponent::FlushTo(SoundEngine* engine) noexcept {
    if (engine == nullptr || !engine->IsRunning()) {
        pending.Clear();
        return;
    }
    SoundMixer& mx = engine->GetMixer();
    for (std::size_t i = 0; i < pending.GetSize(); ++i) {
        const SoundPlayRequest& req = pending[i];
        if (req.spatialBlend > 1.0e-4F) {
            mx.PlayOneShotSpatial(
                    req.clip,
                    req.volume,
                    req.worldPosition,
                    req.spatialBlend,
                    req.minDistance,
                    req.maxDistance);
        } else {
            mx.PlayOneShot(req.clip, req.volume);
        }
    }
    pending.Clear();
}

}  // namespace Spark
