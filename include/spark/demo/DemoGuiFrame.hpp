#pragma once

#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/gui/api/GuiSystem.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark::DemoGui {

/** Extra scale for Spark native immediate-mode demo panels (framebuffer pixels). */
inline constexpr float kImmediateUiScaleBoost = 2.0F;
/** Font and control scale for the Dear ImGui showcase (independent of native demo UI). */
inline constexpr float kImGuiShowcaseUiScale = 1.0F;
/** Design width of right-side tuning panels before <c>immediateUiScale</c> is applied. */
inline constexpr float kDemoSidePanelWidth = 560.0F;
/** Centered launcher panel design width (framebuffer pixels at scale 1). */
inline constexpr float kDemoLauncherPanelWidth = 760.0F;

inline Gui::GuiFrameContext MakeFrameContext(
        IEngineContext& context,
        SceneRenderParams& params,
        const GameWorld& world,
        const float deltaTimeSeconds = 0.0F) {
    Gui::GuiFrameContext frame{};
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) {
        fbW = 1;
    }
    if (fbH <= 0) {
        fbH = 1;
    }
    frame.renderParams = &params;
    frame.input = &context.GetInput();
    if (const auto& font = world.GetUiFont(); font) {
        frame.uiFont = font.Get();
    }
    frame.framebufferWidth = fbW;
    frame.framebufferHeight = fbH;
    frame.deltaTimeSeconds = deltaTimeSeconds;
    frame.immediateUiScale = kImmediateUiScaleBoost;
    return frame;
}

}  // namespace Spark::DemoGui
