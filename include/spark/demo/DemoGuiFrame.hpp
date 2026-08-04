#pragma once

namespace Spark {

class IEngineContext;

namespace DemoGui {

/** Extra scale for Spark native retained demo panels (framebuffer pixels). */
inline constexpr float kImmediateUiScaleBoost = 2.0F;
/** Font and control scale when demos use the Dear ImGui retained backend. */
inline constexpr float kDearImGuiDemoUiScale = 1.0F;
/** Design width of right-side tuning panels before UI scale is applied. */
inline constexpr float kDemoSidePanelWidth = 560.0F;
/** Centered launcher panel design width (framebuffer pixels at scale 1). */
inline constexpr float kDemoLauncherPanelWidth = 760.0F;

/** Select Dear ImGui controls and enable the engine ImGui layer for demo UI. */
void ActivateDearImGuiDemoUi(IEngineContext& context);

/** Tear down demo-only ImGui UI state (does not switch back to Spark native). */
void ShutdownDearImGuiDemoUi(IEngineContext& context) noexcept;

}  // namespace DemoGui
}  // namespace Spark
