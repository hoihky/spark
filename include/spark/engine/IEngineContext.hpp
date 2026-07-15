#pragma once

#include "spark/engine/SceneRenderParams.hpp"

namespace Spark {

class Window;
class IFramePresenter;
class IInput;
class SoundEngine;
class Scene;

/**
 * Facade passed to game code — stable surface that hides Engine internals (Interface Segregation via small types).
 */
class IEngineContext {
public:
    virtual ~IEngineContext() = default;

    [[nodiscard]] virtual Window& GetWindow() = 0;
    [[nodiscard]] virtual IFramePresenter& GetFramePresenter() = 0;
    [[nodiscard]] virtual IInput& GetInput() = 0;

    /** Drawable pixel size for rendering and UI (swapchain when available, else window framebuffer). */
    virtual void GetFramebufferSize(int& outWidth, int& outHeight) const = 0;

    /** Forwards to the frame presenter (Vulkan, etc.); no need to include IFramePresenter. */
    virtual void SetSceneRenderParams(const SceneRenderParams& params) = 0;

    /** Global mixer/output; nullptr only if the host omitted audio construction. */
    [[nodiscard]] virtual SoundEngine* TryGetSoundEngine() noexcept = 0;

    /** Non-owning ECS scene; nullptr when the running game is not a <c>Game</c> (no scene binding). */
    [[nodiscard]] virtual Scene* TryGetScene() noexcept = 0;
};

}  // namespace Spark
