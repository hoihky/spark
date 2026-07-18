#include "spark/ecs/components/audio/SoundCueComponent.hpp"

#include "spark/audio/SoundEngine.hpp"

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

void SoundCueComponent::FlushTo(SoundEngine* engine) noexcept {
    if (engine == nullptr || !engine->IsRunning()) {
        pending.Clear();
        return;
    }
    SoundMixer& mx = engine->GetMixer();
    for (std::size_t i = 0; i < pending.GetSize(); ++i) {
        mx.PlayOneShot(pending[i].clip, pending[i].volume);
    }
    pending.Clear();
}

}  // namespace Spark
