#pragma once

#include "spark/core/Utf8String.hpp"

#include <cstdint>

namespace Spark {

enum class VideoRecordingPreset : std::uint8_t {
    /** Match swapchain / drawable size at record start. */
    Native = 0,
    Hd720 = 1,
    Hd1080 = 2,
};

struct VideoRecordingSettings {
    Utf8String outputPath;
    VideoRecordingPreset preset = VideoRecordingPreset::Native;
    /** Used for CFR timestamps when > 0; 0 falls back to wall-clock frame timing. */
    std::uint32_t fps = 60;
    std::uint32_t videoBitrate = 8'000'000;
    std::uint32_t audioBitrate = 128'000;
    bool applyWatermark = true;

    [[nodiscard]] static std::uint32_t PresetWidth(VideoRecordingPreset preset) noexcept {
        switch (preset) {
            case VideoRecordingPreset::Hd720:
                return 1280;
            case VideoRecordingPreset::Hd1080:
                return 1920;
            default:
                return 0;
        }
    }

    [[nodiscard]] static std::uint32_t PresetHeight(VideoRecordingPreset preset) noexcept {
        switch (preset) {
            case VideoRecordingPreset::Hd720:
                return 720;
            case VideoRecordingPreset::Hd1080:
                return 1080;
            default:
                return 0;
        }
    }
};

}  // namespace Spark
