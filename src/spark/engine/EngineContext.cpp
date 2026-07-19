#include "spark/engine/EngineContext.hpp"

#include "spark/audio/SoundEngine.hpp"
#include "spark/engine/IFramePresenter.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/scene/Scene.hpp"

namespace Spark {

EngineContext::EngineContext(Window& w, IFramePresenter& p, IInput& i, SoundEngine& a)
    : window(w), presenter(p), input(i), audio(a) {}

void EngineContext::SetSceneBinding(Scene* scene) noexcept {
    sceneBinding = scene;
}

void EngineContext::BindImGuiLayer(IImGuiLayer& layer) noexcept {
    imguiLayerBinding = &layer;
}

Window& EngineContext::GetWindow() {
    return window;
}

IFramePresenter& EngineContext::GetFramePresenter() {
    return presenter;
}

IInput& EngineContext::GetInput() {
    return input;
}

void EngineContext::GetFramebufferSize(int& outWidth, int& outHeight) const {
    if (presenter.TryGetDrawableSize(outWidth, outHeight)) {
        return;
    }
    window.GetFramebufferSize(outWidth, outHeight);
}

void EngineContext::SetSceneRenderParams(const SceneRenderParams& params) {
    presenter.SetSceneRenderParams(params);
}

SoundEngine* EngineContext::TryGetSoundEngine() noexcept {
    return &audio;
}

Scene* EngineContext::TryGetScene() noexcept {
    return sceneBinding;
}

IImGuiLayer* EngineContext::TryGetImGuiLayer() noexcept {
    return imguiLayerBinding;
}

}  // namespace Spark
