#include "spark/engine/Engine.hpp"
#include "spark/engine/EngineContext.hpp"
#include "spark/engine/Game.hpp"
#include "spark/engine/GlfwInput.hpp"
#include "spark/engine/IFramePresenter.hpp"
#include "spark/engine/IRenderFrame.hpp"
#include "spark/engine/ISceneProvider.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/media/VideoRecordingSettings.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/render/core/VulkanRenderer.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/config.hpp"
#include "spark/core/Utf8String.hpp"

#include <GLFW/glfw3.h>

#include <chrono>
#include <cstdio>
#include <ctime>

namespace Spark {

namespace {

struct EmptyRenderFrame final : IRenderFrame {};

Utf8String BuildTimestampedRecordingPath() {
    char path[512]{};
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowSec = std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &nowSec);
#else
    localtime_r(&nowSec, &localTm);
#endif
    std::snprintf(
            path,
            sizeof(path),
            "%s/recordings/spark_%04d%02d%02d_%02d%02d%02d.mp4",
            SPARK_BUILD_ASSETS_DIR,
            localTm.tm_year + 1900,
            localTm.tm_mon + 1,
            localTm.tm_mday,
            localTm.tm_hour,
            localTm.tm_min,
            localTm.tm_sec);
    return Utf8String(path);
}

VideoRecordingSettings RecordingSettingsFromRunOptions(const EngineRunOptions& options, const Utf8String& outputPath) {
    VideoRecordingSettings settings{};
    settings.outputPath = outputPath;
    settings.preset = options.recordPreset;
    settings.fps = options.recordFps > 0 ? options.recordFps : 60;
    settings.videoBitrate = options.videoBitrate;
    settings.audioBitrate = options.audioBitrate;
    settings.applyWatermark = options.recordWatermark;
    return settings;
}

bool StartVideoRecording(IFramePresenter& presenter, SoundEngine& audio, const VideoRecordingSettings& settings) {
    if (presenter.IsVideoRecording()) {
        return true;
    }
    if (!presenter.BeginVideoRecording(settings)) {
        std::fprintf(stderr, "Spark: video recording is unavailable on this platform\n");
        return false;
    }
    audio.SetVideoRecorder(presenter.GetActiveVideoRecorder());
    return true;
}

void StopVideoRecording(IFramePresenter& presenter, SoundEngine& audio) {
    if (!presenter.IsVideoRecording()) {
        return;
    }
    presenter.EndVideoRecording();
    audio.SetVideoRecorder(nullptr);
}

}  // namespace

struct Engine::Impl {
    Window window;
    UniquePtr<VulkanRenderer> renderer;
    UniquePtr<GlfwInput> input;
    SoundEngine audio{};
    UniquePtr<EngineContext> engineContext;
    UniquePtr<IGame> game;
    EngineRunOptions runOptions{};
    bool autoRecordPending = false;

    explicit Impl(UniquePtr<IGame> ownedGame)
        : window{}
        , renderer(MakeUnique<VulkanRenderer>(window))
        , input(MakeUnique<GlfwInput>(window.Handle()))
        , engineContext(MakeUnique<EngineContext>(window, *renderer, *input, audio))
        , game(MoveTemp(ownedGame)) {
        input->WireToWindow(window);
        window.SetFramebufferResizeCallback([this](int /*width*/, int /*height*/) {
            renderer->NotifySwapchainResize();
        });
    }

    void Run() {
        if (!game) {
            return;
        }
        autoRecordPending = !runOptions.autoRecordPath.IsEmpty();
        EmptyRenderFrame renderFrame;
        if (ISceneProvider* sceneHost = dynamic_cast<ISceneProvider*>(game.Get())) {
            engineContext->SetSceneBinding(&sceneHost->GetScene());
        }
        game->OnAttach(*engineContext);
        (void)audio.Startup();

        using Clock = std::chrono::steady_clock;
        auto previous = Clock::now();
        float totalSeconds = 0.0F;
        std::uint64_t frameIndex = 0;

        while (!window.ShouldClose()) {
            window.PollEvents();
            input->BeginFrame();

            if (input->IsKeyPressedThisFrame(GLFW_KEY_F12)) {
                char shotPath[512]{};
                const auto now = std::chrono::system_clock::now();
                const std::time_t nowSec = std::chrono::system_clock::to_time_t(now);
                std::tm localTm{};
#if defined(_WIN32)
                localtime_s(&localTm, &nowSec);
#else
                localtime_r(&nowSec, &localTm);
#endif
                std::snprintf(
                        shotPath,
                        sizeof(shotPath),
                        "%s/screenshots/spark_%04d%02d%02d_%02d%02d%02d.png",
                        SPARK_BUILD_ASSETS_DIR,
                        localTm.tm_year + 1900,
                        localTm.tm_mon + 1,
                        localTm.tm_mday,
                        localTm.tm_hour,
                        localTm.tm_min,
                        localTm.tm_sec);
                engineContext->GetFramePresenter().RequestScreenshotSave(shotPath);
            }

            if (input->IsKeyPressedThisFrame(GLFW_KEY_F9)) {
                IFramePresenter& presenter = engineContext->GetFramePresenter();
                if (presenter.IsVideoRecording()) {
                    StopVideoRecording(presenter, audio);
                } else {
                    const VideoRecordingSettings settings =
                            RecordingSettingsFromRunOptions(runOptions, BuildTimestampedRecordingPath());
                    (void)StartVideoRecording(presenter, audio, settings);
                }
            }

            if (autoRecordPending) {
                const VideoRecordingSettings settings =
                        RecordingSettingsFromRunOptions(runOptions, runOptions.autoRecordPath);
                if (StartVideoRecording(engineContext->GetFramePresenter(), audio, settings)) {
                    autoRecordPending = false;
                }
            }

            const auto now = Clock::now();
            const float deltaSeconds = std::chrono::duration<float>(now - previous).count();
            previous = now;
            totalSeconds += deltaSeconds;

            const FrameTiming timing{deltaSeconds, totalSeconds, frameIndex};
            game->OnUpdate(timing, *engineContext);
            audio.PumpFrame(timing.deltaTimeSeconds);
            game->OnRender(renderFrame, *engineContext);
            renderer->PresentFrame();
            ++frameIndex;
        }

        StopVideoRecording(engineContext->GetFramePresenter(), audio);
        game->OnDetach();
        audio.Shutdown();
    }
};

Engine::Engine(UniquePtr<IGame> game) : impl(MakeUnique<Impl>(MoveTemp(game))) {}

Engine::~Engine() = default;

void Engine::Run(const EngineRunOptions& options) {
    impl->runOptions = options;
    impl->Run();
}

}  // namespace Spark
