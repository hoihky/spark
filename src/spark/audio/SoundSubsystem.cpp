#include "spark/audio/SoundSubsystem.hpp"

#include "spark/audio/AmbientAudio.hpp"
#include "spark/audio/AudioSpatial.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void ProcessSoundCues(GameWorld& world, IEngineContext& context) {
    ProcessAudioListeners(world);
    ProcessAmbientZones(world);
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

}  // namespace Spark
