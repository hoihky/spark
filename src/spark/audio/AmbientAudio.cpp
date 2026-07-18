#include "spark/audio/AmbientAudio.hpp"

#include "spark/audio/AudioListenerPose.hpp"
#include "spark/ecs/components/audio/AmbientZoneComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/VolumeRegions.hpp"

namespace Spark {

namespace {

AmbientAudioMix gFrameMix{};

}  // namespace

AmbientAudioMix& GetFrameAmbientAudioMix() noexcept {
    return gFrameMix;
}

const AmbientAudioMix& GetFrameAmbientAudioMixConst() noexcept {
    return gFrameMix;
}

void ProcessAmbientZones(const GameWorld& world) noexcept {
    gFrameMix = AmbientAudioMix{};
    const AudioListenerPose& listener = GetFrameAudioListenerPoseConst();
    if (!listener.valid) {
        return;
    }

    bool found = false;
    std::int32_t bestPriority = 0;
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const AmbientZoneComponent* zone = o->GetComponent<AmbientZoneComponent>();
        if (zone == nullptr || !zone->IsEnabled()) {
            return;
        }
        if (!PointInsideVolume(listener.position, *o, zone->GetShape(), zone->GetHalfExtents())) {
            return;
        }
        if (!found || zone->GetPriority() > bestPriority) {
            found = true;
            bestPriority = zone->GetPriority();
            gFrameMix.active = true;
            gFrameMix.volumeScale = zone->GetVolumeScale();
            gFrameMix.lowPassAmount = zone->GetLowPassAmount();
        }
    });
}

}  // namespace Spark
