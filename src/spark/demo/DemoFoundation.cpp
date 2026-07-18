#include "spark/demo/DemoFoundation.hpp"

#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

void DemoRootCollection::Clear() noexcept {
    roots.Clear();
}

void DemoRootCollection::Track(GameObject* const object) noexcept {
    if (object != nullptr) {
        roots.PushBack(object);
    }
}

void DemoRootCollection::DestroyAll(GameWorld& world) noexcept {
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
}

float DemoSmoothedFps::Update(const float deltaSeconds, const std::uint32_t frameIndex) noexcept {
    const float instant = (deltaSeconds > 1.0e-6F) ? (1.0F / deltaSeconds) : 0.0F;
    if (frameIndex < 2U) {
        smoothed = instant;
    } else {
        smoothed = smoothed * 0.88F + instant * 0.12F;
    }
    return smoothed;
}

namespace DemoAudio {

void QueueCue(GameObject& actor, const SharedPtr<SoundClip>& clip, const float volume) noexcept {
    if (!clip) {
        return;
    }
    SoundCueComponent* cue = actor.GetComponent<SoundCueComponent>();
    if (cue == nullptr) {
        cue = actor.AddComponent<SoundCueComponent>();
    }
    cue->Queue(clip, volume);
}

void QueueCueAtWorld(
        GameObject& actor,
        const SharedPtr<SoundClip>& clip,
        const Vector3& worldPosition,
        const float volume,
        const float spatialBlend) noexcept {
    if (!clip) {
        return;
    }
    SoundCueComponent* cue = actor.GetComponent<SoundCueComponent>();
    if (cue == nullptr) {
        cue = actor.AddComponent<SoundCueComponent>();
    }
    cue->QueueAtWorld(clip, volume, worldPosition, spatialBlend);
}

}  // namespace DemoAudio

namespace DemoHud {

void Apply(TextOverlayComponent& overlay, const bool lightTextOnDarkBackground) noexcept {
    overlay.SetFontSizePixels(kFontSizePixels);
    overlay.SetColor(lightTextOnDarkBackground ? kLightTextColor : kDarkTextColor);
    overlay.SetAlpha(kTextAlpha);
}

}  // namespace DemoHud

}  // namespace Spark
