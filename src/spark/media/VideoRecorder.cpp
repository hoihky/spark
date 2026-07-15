#include "spark/media/VideoRecorder.hpp"

#include "spark/memory/UniquePtr.hpp"

#if defined(SPARK_PLATFORM_APPLE)

namespace Spark {

UniquePtr<VideoRecorder> CreateMacVideoRecorder();

}

#endif

namespace Spark {

#if defined(SPARK_PLATFORM_APPLE)

UniquePtr<VideoRecorder> VideoRecorder::Create() {
    return CreateMacVideoRecorder();
}

#else

UniquePtr<VideoRecorder> VideoRecorder::Create() {
    class NullVideoRecorder final : public VideoRecorder {};
    return UniquePtr<VideoRecorder>(new NullVideoRecorder());
}

#endif

bool VideoRecorder::Begin(
        const VideoRecordingSettings& /*settings*/,
        const std::uint32_t /*captureWidth*/,
        const std::uint32_t /*captureHeight*/) noexcept {
    return false;
}

void VideoRecorder::AppendVideoFrameBgra(
        const std::uint8_t* /*bgra*/,
        const std::uint32_t /*width*/,
        const std::uint32_t /*height*/,
        const double /*ptsSeconds*/) noexcept {}

void VideoRecorder::AppendAudioInterleavedFloat(const float* /*samples*/, const std::size_t /*frameCount*/) noexcept {}

bool VideoRecorder::End() noexcept {
    return false;
}

bool VideoRecorder::IsActive() const noexcept {
    return false;
}

}  // namespace Spark
