#include "spark/audio/SoundSubsystem.hpp"

#include "spark/audio/AmbientAudio.hpp"
#include "spark/audio/AudioSpatial.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void SoundSubsystem::UpdateListeners(const GameWorld& world) noexcept {
    ProcessAudioListeners(world);
}

void SoundSubsystem::UpdateAmbient(const GameWorld& world) noexcept {
    ProcessAmbientZones(world);
}

void SoundSubsystem::DrainSoundCues(GameWorld& world, IEngineContext& context) {
    SoundEngine* audio = context.TryGetSoundEngine();
    if (audio == nullptr || !audio->IsRunning()) {
        return;
    }
    world.ForEachActiveGameObject([audio](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        SoundCueComponent* sc = o->GetComponent<SoundCueComponent>();
        if (sc != nullptr) {
            sc->FlushTo(audio);
        }
    });
}

void SoundSubsystem::ProcessFrame(GameWorld& world, IEngineContext& context) {
    UpdateListeners(world);
    UpdateAmbient(world);
    DrainSoundCues(world, context);
}

void ProcessSoundCues(GameWorld& world, IEngineContext& context) {
    SoundSubsystem::ProcessFrame(world, context);
}

}  // namespace Spark
