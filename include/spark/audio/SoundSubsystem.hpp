#pragma once

namespace Spark {

class GameWorld;
class IEngineContext;

/** Drains <c>SoundCueComponent</c> queues after component <c>OnUpdate</c> runs. */
void ProcessSoundCues(GameWorld& world, IEngineContext& context);

}  // namespace Spark
