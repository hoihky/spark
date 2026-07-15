#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/media/VideoRecordingSettings.hpp"

namespace Spark {

/** Optional main-loop configuration (CLI flags, auto-record, etc.). */
struct EngineRunOptions {
    /** When non-empty, recording starts immediately on the first frame. */
    Utf8String autoRecordPath;
    VideoRecordingPreset recordPreset = VideoRecordingPreset::Native;
    std::uint32_t recordFps = 60;
    std::uint32_t videoBitrate = 8'000'000;
    std::uint32_t audioBitrate = 128'000;
    bool recordWatermark = true;
};

}  // namespace Spark
