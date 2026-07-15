#pragma once

#include "spark/engine/SceneRenderParams.hpp"

namespace Spark {

struct VideoRecordingSettings;
class VideoRecorder;

/**
 * Swapchain / frame presentation (Dependency Inversion — Engine does not depend on Vulkan types here).
 */
class IFramePresenter {
public:
    virtual ~IFramePresenter() = default;

    /** Submits the current frame to the display (e.g. Vulkan present). */
    virtual void PresentFrame() = 0;

    /** Called when the window framebuffer size changes. */
    virtual void NotifySwapchainResize() = 0;

    /**
     * Pixel size of the drawable actually rendered (e.g. swapchain). When true, matches UI projection / hit tests.
     * Default: unknown — use Window / GLFW framebuffer size instead.
     */
    [[nodiscard]] virtual bool TryGetDrawableSize(int& outWidth, int& outHeight) const {
        (void)outWidth;
        (void)outHeight;
        return false;
    }

    /** Optional: Vulkan backend uses this for lit meshes; default is no-op. */
    virtual void SetSceneRenderParams(const SceneRenderParams& params) {
        (void)params;
    }

    /**
     * Request a PNG capture of the next presented frame (swapchain after UI).
     * Default no-op; VulkanRenderer implements readback (use F12 in Engine on macOS).
     */
    virtual void RequestScreenshotSave(const char* pathUtf8) {
        (void)pathUtf8;
    }

    /** Begin H.264 + AAC MP4 capture of swapchain frames (macOS AVAssetWriter). */
    [[nodiscard]] virtual bool BeginVideoRecording(const VideoRecordingSettings& settings) {
        (void)settings;
        return false;
    }

    virtual void EndVideoRecording() {}

    [[nodiscard]] virtual bool IsVideoRecording() const { return false; }

    /** Active recorder while <c>IsVideoRecording()</c>; used to tap mixed audio. */
    [[nodiscard]] virtual VideoRecorder* GetActiveVideoRecorder() { return nullptr; }
};

}  // namespace Spark
