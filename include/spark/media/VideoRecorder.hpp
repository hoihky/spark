#pragma once

#include "spark/media/VideoRecordingSettings.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Platform video recorder (macOS: AVAssetWriter H.264 + AAC MP4).
 * Append video as BGRA8 rows and audio as 48 kHz stereo float interleaved.
 */
class VideoRecorder {
public:
    [[nodiscard]] static UniquePtr<VideoRecorder> Create();

    VideoRecorder() = default;
    virtual ~VideoRecorder() = default;

    VideoRecorder(const VideoRecorder&) = delete;
    VideoRecorder& operator=(const VideoRecorder&) = delete;

    /** @param captureWidth/@param captureHeight Swapchain size used to resolve Native preset. */
    [[nodiscard]] virtual bool Begin(
            const VideoRecordingSettings& settings,
            std::uint32_t captureWidth,
            std::uint32_t captureHeight) noexcept;

    /** BGRA8, tightly packed rows (width * 4 bytes per row). @p ptsSeconds presentation time. */
    virtual void AppendVideoFrameBgra(
            const std::uint8_t* bgra,
            std::uint32_t width,
            std::uint32_t height,
            double ptsSeconds) noexcept;

    /** 48 kHz stereo interleaved float PCM. */
    virtual void AppendAudioInterleavedFloat(const float* samples, std::size_t frameCount) noexcept;

    /** Finalize MP4 on disk. Safe to call when inactive. */
    [[nodiscard]] virtual bool End() noexcept;

    [[nodiscard]] virtual bool IsActive() const noexcept;

    [[nodiscard]] std::uint32_t OutputWidth() const noexcept { return outputWidth; }
    [[nodiscard]] std::uint32_t OutputHeight() const noexcept { return outputHeight; }

protected:
    std::uint32_t outputWidth = 0;
    std::uint32_t outputHeight = 0;
};

}  // namespace Spark
