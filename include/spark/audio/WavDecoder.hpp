#pragma once

#include "spark/audio/SoundClip.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

/** Loads PCM WAV (16-bit LE, mono or stereo) into stereo float interleaved at native rate. For Ogg/MP3 use
 * <c>TryLoadSoundClipFromFile</c> in <c>spark/audio/SoundFileLoader.hpp</c>. */
[[nodiscard]] SharedPtr<SoundClip> TryLoadSoundClipFromWavFile(const char* path);

}  // namespace Spark
