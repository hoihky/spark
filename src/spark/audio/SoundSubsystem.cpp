#include "spark/audio/SoundSubsystem.hpp"

#include "spark/audio/SoundEngine.hpp"
#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void ProcessSoundCues(GameWorld& world, IEngineContext& context) {
    SoundEngine* audio = context.TryGetSoundEngine();
    if (audio == nullptr || !audio->IsRunning()) {
        return;
    }
    world.ForEachGameObject([audio](GameObject* o) {
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
