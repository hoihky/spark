#include "spark/engine/Engine.hpp"
#include "spark/engine/IFramePresenter.hpp"
#include "spark/engine/Game.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/IRenderFrame.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/media/VideoRecordingSettings.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "../include/spark/render/VulkanRenderer.hpp"
#include "../include/spark/render/Window.hpp"
#include "spark/config.hpp"

#include <GLFW/glfw3.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

#if defined(__APPLE__)
#define SPARK_GLFW_USE_CURSOR_CALLBACK 0
#else
#define SPARK_GLFW_USE_CURSOR_CALLBACK 1
#endif

namespace Spark {

namespace {

class GlfwInput final : public IInput {
public:
    explicit GlfwInput(GLFWwindow* w) : window(w) { Clear(); }

    /** Must be called once after the Window exists. Non-Apple: cursor callback (e.g. Wayland). macOS: poll only. */
    void WireToWindow([[maybe_unused]] Window& win) {
#if SPARK_GLFW_USE_CURSOR_CALLBACK
        win.SetCursorPosCallback([this](double x, double y) {
            cursorXWin = x;
            cursorYWin = y;
        });
#endif
        win.SetScrollCallback([this](double /*xoffset*/, double yoffset) { scrollAccumY += yoffset; });
        glfwGetCursorPos(window, &cursorXWin, &cursorYWin);
    }

    void BeginFrame() {
        scrollDeltaThisFrame = static_cast<float>(scrollAccumY);
        scrollAccumY = 0.0;
        for (int k = 0; k <= GLFW_KEY_LAST; ++k) {
            prev[static_cast<unsigned>(k)] = curr[static_cast<unsigned>(k)];
            const int s = glfwGetKey(window, k);
            curr[static_cast<unsigned>(k)] = (s == GLFW_PRESS || s == GLFW_REPEAT);
        }
        for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b) {
            mousePrev[static_cast<unsigned>(b)] = mouseCurr[static_cast<unsigned>(b)];
            mouseCurr[static_cast<unsigned>(b)] = (glfwGetMouseButton(window, b) == GLFW_PRESS);
        }

        double mx = 0.0;
        double my = 0.0;
        if (cursorCaptured) {
            glfwGetCursorPos(window, &mx, &my);
            mouseDeltaX = static_cast<float>(mx - lastMouseX);
            mouseDeltaY = static_cast<float>(my - lastMouseY);
            int winW = 0;
            int winH = 0;
            glfwGetWindowSize(window, &winW, &winH);
            if (winW > 0 && winH > 0) {
                const double cx = static_cast<double>(winW) * 0.5;
                const double cy = static_cast<double>(winH) * 0.5;
                glfwSetCursorPos(window, cx, cy);
                // Baseline = reported pos after warp, not ideal center: macOS + screen recording can offset
                // GetCursorPos vs SetCursorPos; assuming center yields biased deltas and downward look drift.
                glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
            }
            // Ignore tiny deltas from sub-pixel noise or capture overlays while the cursor is disabled.
            constexpr float kCapturedMouseDeadZone = 0.12F;
            if (std::fabs(mouseDeltaX) < kCapturedMouseDeadZone) {
                mouseDeltaX = 0.0F;
            }
            if (std::fabs(mouseDeltaY) < kCapturedMouseDeadZone) {
                mouseDeltaY = 0.0F;
            }
            if (glfwRawMouseMotionSupported() != 0) {
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }
        } else {
#if SPARK_GLFW_USE_CURSOR_CALLBACK
            mx = cursorXWin;
            my = cursorYWin;
#else
            glfwGetCursorPos(window, &mx, &my);
#endif
            mouseDeltaX = static_cast<float>(mx - lastMouseX);
            mouseDeltaY = static_cast<float>(my - lastMouseY);
            lastMouseX = mx;
            lastMouseY = my;
        }
    }

    [[nodiscard]] bool IsKeyDown(int keyCode) const override {
        if (keyCode < 0 || keyCode > GLFW_KEY_LAST) {
            return false;
        }
        return curr[static_cast<unsigned>(keyCode)];
    }

    [[nodiscard]] bool IsKeyPressedThisFrame(int keyCode) const override {
        if (keyCode < 0 || keyCode > GLFW_KEY_LAST) {
            return false;
        }
        const unsigned u = static_cast<unsigned>(keyCode);
        return curr[u] && !prev[u];
    }

    [[nodiscard]] float GetMouseDeltaX() const override { return mouseDeltaX; }
    [[nodiscard]] float GetMouseDeltaY() const override { return mouseDeltaY; }

    [[nodiscard]] bool IsMouseButtonDown(int button) const override {
        if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
            return false;
        }
        return mouseCurr[static_cast<unsigned>(button)];
    }

    [[nodiscard]] bool IsMouseButtonPressedThisFrame(int button) const override {
        if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
            return false;
        }
        const unsigned u = static_cast<unsigned>(button);
        return mouseCurr[u] && !mousePrev[u];
    }

    [[nodiscard]] bool IsMouseButtonReleasedThisFrame(int button) const override {
        if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) {
            return false;
        }
        const unsigned u = static_cast<unsigned>(button);
        return !mouseCurr[u] && mousePrev[u];
    }

    void GetCursorFramebufferPixels(float& outX, float& outY, int drawableWidth, int drawableHeight) const override {
        int winW = 0;
        int winH = 0;
        glfwGetWindowSize(window, &winW, &winH);
        int fbW = 0;
        int fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (drawableWidth > 0 && drawableHeight > 0) {
            fbW = drawableWidth;
            fbH = drawableHeight;
        }
        double cx = 0.0;
        double cy = 0.0;
        if (cursorCaptured) {
            glfwGetCursorPos(window, &cx, &cy);
        } else {
#if SPARK_GLFW_USE_CURSOR_CALLBACK
            cx = cursorXWin;
            cy = cursorYWin;
#else
            glfwGetCursorPos(window, &cx, &cy);
#endif
        }
        if (winW > 0 && winH > 0 && fbW > 0 && fbH > 0) {
            outX = static_cast<float>(cx * static_cast<double>(fbW) / static_cast<double>(winW));
            outY = static_cast<float>(cy * static_cast<double>(fbH) / static_cast<double>(winH));
        } else {
            outX = static_cast<float>(cx);
            outY = static_cast<float>(cy);
        }
    }

    void SetCursorCaptured(bool capture) override {
        if (cursorCaptured == capture) {
            if (!capture) {
#if !SPARK_GLFW_USE_CURSOR_CALLBACK
                glfwGetCursorPos(window, &cursorXWin, &cursorYWin);
#endif
            }
            return;
        }
        cursorCaptured = capture;
        glfwSetInputMode(window, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (glfwRawMouseMotionSupported() != 0) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, capture ? GLFW_TRUE : GLFW_FALSE);
        }
        int winW = 0;
        int winH = 0;
        glfwGetWindowSize(window, &winW, &winH);
        if (winW > 0 && winH > 0) {
            if (capture) {
                const double cx = static_cast<double>(winW) * 0.5;
                const double cy = static_cast<double>(winH) * 0.5;
                glfwSetCursorPos(window, cx, cy);
                glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
            } else {
                glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
            }
        }
        if (!capture) {
            glfwGetCursorPos(window, &cursorXWin, &cursorYWin);
        }
        mouseDeltaX = 0.0F;
        mouseDeltaY = 0.0F;
    }

    [[nodiscard]] bool IsCursorCaptured() const override { return cursorCaptured; }

    [[nodiscard]] float GetScrollDeltaY() const override { return scrollDeltaThisFrame; }

    [[nodiscard]] bool TryGetClipboardUtf8(Utf8String& out) const override {
        const char* c = glfwGetClipboardString(window);
        if (c == nullptr || c[0] == '\0') {
            return false;
        }
        out = Utf8String(c);
        return true;
    }

    void SetClipboardUtf8(const Utf8String& text) const override {
        glfwSetClipboardString(window, text.CStr());
    }

private:
    void Clear() {
        std::memset(curr, 0, sizeof(curr));
        std::memset(prev, 0, sizeof(prev));
        std::memset(mouseCurr, 0, sizeof(mouseCurr));
        std::memset(mousePrev, 0, sizeof(mousePrev));
    }

    GLFWwindow* window = nullptr;
    bool curr[GLFW_KEY_LAST + 1U]{};
    bool prev[GLFW_KEY_LAST + 1U]{};
    bool mouseCurr[GLFW_MOUSE_BUTTON_LAST + 1U]{};
    bool mousePrev[GLFW_MOUSE_BUTTON_LAST + 1U]{};
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    double cursorXWin = 0.0;
    double cursorYWin = 0.0;
    float mouseDeltaX = 0.0F;
    float mouseDeltaY = 0.0F;
    bool cursorCaptured = false;
    double scrollAccumY = 0.0;
    float scrollDeltaThisFrame = 0.0F;
};

class EngineContextImpl final : public IEngineContext {
public:
    EngineContextImpl(Window& w, IFramePresenter& p, IInput& i, SoundEngine& a) : win(w), presenter(p), inp(i), audio(a) {}

    void SetSceneBinding(Scene* s) noexcept { sceneBinding = s; }

    [[nodiscard]] Window& GetWindow() override { return win; }
    [[nodiscard]] IFramePresenter& GetFramePresenter() override { return presenter; }
    [[nodiscard]] IInput& GetInput() override { return inp; }

    void GetFramebufferSize(int& outWidth, int& outHeight) const override {
        if (presenter.TryGetDrawableSize(outWidth, outHeight)) {
            return;
        }
        win.GetFramebufferSize(outWidth, outHeight);
    }

    void SetSceneRenderParams(const SceneRenderParams& params) override {
        presenter.SetSceneRenderParams(params);
    }

    [[nodiscard]] SoundEngine* TryGetSoundEngine() noexcept override { return &audio; }

    [[nodiscard]] Scene* TryGetScene() noexcept override { return sceneBinding; }

private:
    Window& win;
    IFramePresenter& presenter;
    IInput& inp;
    SoundEngine& audio;
    Scene* sceneBinding = nullptr;
};

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
    UniquePtr<EngineContextImpl> engineContext;
    UniquePtr<IGame> game;
    EngineRunOptions runOptions{};
    bool autoRecordPending = false;

    explicit Impl(UniquePtr<IGame> ownedGame)
        : window{}
        , renderer(MakeUnique<VulkanRenderer>(window))
        , input(MakeUnique<GlfwInput>(window.Handle()))
        , engineContext(MakeUnique<EngineContextImpl>(window, *renderer, *input, audio))
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
        if (Game* concreteGame = dynamic_cast<Game*>(game.Get())) {
            engineContext->SetSceneBinding(&concreteGame->GetScene());
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
