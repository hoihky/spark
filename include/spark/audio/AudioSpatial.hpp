#pragma once

namespace Spark {

class GameWorld;

/** Resolves the highest-priority enabled <c>AudioListenerComponent</c> into <c>GetFrameAudioListenerPose()</c>. */
void ProcessAudioListeners(const GameWorld& world) noexcept;

}  // namespace Spark
