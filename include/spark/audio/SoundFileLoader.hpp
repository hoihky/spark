#pragma once

#include "spark/audio/SoundClip.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

/**
 * Dispatches on file extension to decoders (WAV / <c>.wave</c> PCM, Ogg Vorbis via stb_vorbis, MP3 via minimp3).
 * Add new formats by extending the implementation TU behind this entry point (Open/Closed: callers stay on this API).
 */
[[nodiscard]] SharedPtr<SoundClip> TryLoadSoundClipFromFile(const char* utf8Path);

/**
 * Resolves audio under the project <c>assets/</c> tree using <c>SPARK_ASSETS_DIR</c> (CMake absolute path), so loads
 * work when the process CWD is the build folder. Tries: <c>SPARK_ASSETS_DIR</c>/<suffix>,
 * <c>SPARK_BUILD_ASSETS_DIR</c>/<suffix>, then <c>assetPathUtf8</c> as a normal relative path.
 * Pass <c>"assets/audio/foo.wav"</c> or <c>"audio/foo.wav"</c> (both resolve under <c>assets/</c>).
 */
[[nodiscard]] SharedPtr<SoundClip> TryLoadSoundClipFromBundledAsset(const char* assetPathUtf8);

}  // namespace Spark
