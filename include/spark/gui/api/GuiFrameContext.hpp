#pragma once

namespace Spark {

class Font;
class IInput;
struct SceneRenderParams;

namespace Gui {

/** Per-frame data for portable immediate-mode UI (both backends). */
struct GuiFrameContext {
    SceneRenderParams* renderParams = nullptr;
    IInput* input = nullptr;
    const Font* uiFont = nullptr;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    float contentScaleX = 1.0F;
    float contentScaleY = 1.0F;
    float deltaTimeSeconds = 0.0F;
    /** Multiplier on top of <c>GetEffectiveGuiUiScale()</c> for portable immediate UI (demos use &gt; 1). */
    float immediateUiScale = 1.0F;
};

}  // namespace Gui
}  // namespace Spark
