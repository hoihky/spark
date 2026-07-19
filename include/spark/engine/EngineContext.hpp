#pragma once

#include "spark/engine/IEngineContext.hpp"

namespace Spark {

class Window;
class IFramePresenter;
class IInput;
class IImGuiLayer;
class SoundEngine;
class Scene;

/** Default <c>IEngineContext</c> implementation used by <c>Engine</c> (Facade). */
class EngineContext final : public IEngineContext {
public:
    EngineContext(Window& window, IFramePresenter& presenter, IInput& input, SoundEngine& audio);

    void SetSceneBinding(Scene* scene) noexcept;
    void BindImGuiLayer(IImGuiLayer& layer) noexcept;

    [[nodiscard]] Window& GetWindow() override;
    [[nodiscard]] IFramePresenter& GetFramePresenter() override;
    [[nodiscard]] IInput& GetInput() override;
    void GetFramebufferSize(int& outWidth, int& outHeight) const override;
    void SetSceneRenderParams(const SceneRenderParams& params) override;
    [[nodiscard]] SoundEngine* TryGetSoundEngine() noexcept override;
    [[nodiscard]] Scene* TryGetScene() noexcept override;
    [[nodiscard]] IImGuiLayer* TryGetImGuiLayer() noexcept override;

private:
    Window& window;
    IFramePresenter& presenter;
    IInput& input;
    SoundEngine& audio;
    Scene* sceneBinding = nullptr;
    IImGuiLayer* imguiLayerBinding = nullptr;
};

}  // namespace Spark
