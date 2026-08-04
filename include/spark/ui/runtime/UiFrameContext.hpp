#pragma once

namespace Spark {

class Font;
class IInput;
struct SceneRenderParams;

namespace Ui {

/** Per-frame context passed to backends when building or painting UI. */
struct UiFrameContext {
    SceneRenderParams* renderParams = nullptr;
    IInput* input = nullptr;
    const Font* uiFont = nullptr;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    float contentScaleX = 1.0F;
    float contentScaleY = 1.0F;
    float deltaTimeSeconds = 0.0F;
    float immediateUiScale = 1.0F;
};

}  // namespace Ui
}  // namespace Spark
