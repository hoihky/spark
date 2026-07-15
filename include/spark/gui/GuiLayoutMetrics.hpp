#pragma once

namespace Spark::Gui {

/**
 * Scalable layout constants for retained-mode GUI (padding, row heights, font sizes).
 * Base values are defined at <c>uiScale == 1</c>; multiply by <c>uiScale</c> for DPI / editor zoom.
 */
struct GuiLayoutMetrics {
    /** Global UI scale (DPI factor or editor zoom). Default 1.0. */
    float uiScale = 1.0F;

    float panelPadding = 8.0F;
    float groupPadding = 10.0F;
    float formRowHeight = 28.0F;
    float listRowHeight = 30.0F;
    float treeRowHeight = 28.0F;
    float dropdownRowHeight = 28.0F;
    float contextMenuRowHeight = 32.0F;
    float menuBarHeight = 34.0F;
    float controlGap = 4.0F;
    float fontBody = 18.0F;
    float fontControl = 22.0F;
    float fontLabel = 22.0F;
    float fontSmall = 15.0F;
    float textBoxFont = 20.0F;

    [[nodiscard]] float Scaled(float base) const noexcept { return base * uiScale; }

    [[nodiscard]] float Padding() const noexcept { return Scaled(panelPadding); }
    [[nodiscard]] float GroupPadding() const noexcept { return Scaled(groupPadding); }
    [[nodiscard]] float FormRowHeight() const noexcept { return Scaled(formRowHeight); }
    [[nodiscard]] float ListRowHeight() const noexcept { return Scaled(listRowHeight); }
    [[nodiscard]] float TreeRowHeight() const noexcept { return Scaled(treeRowHeight); }
    [[nodiscard]] float DropdownRowHeight() const noexcept { return Scaled(dropdownRowHeight); }
    [[nodiscard]] float ContextMenuRowHeight() const noexcept { return Scaled(contextMenuRowHeight); }
    [[nodiscard]] float MenuBarHeight() const noexcept { return Scaled(menuBarHeight); }
    [[nodiscard]] float ControlGap() const noexcept { return Scaled(controlGap); }
    [[nodiscard]] float FontBody() const noexcept { return Scaled(fontBody); }
    [[nodiscard]] float FontControl() const noexcept { return Scaled(fontControl); }
    [[nodiscard]] float FontLabel() const noexcept { return Scaled(fontLabel); }
    [[nodiscard]] float FontSmall() const noexcept { return Scaled(fontSmall); }
    [[nodiscard]] float TextBoxFont() const noexcept { return Scaled(textBoxFont); }

    [[nodiscard]] static GuiLayoutMetrics Default() noexcept;
};

/** Active metrics for the current input/paint frame (set from the front canvas). */
[[nodiscard]] const GuiLayoutMetrics& GetActiveGuiLayoutMetrics() noexcept;
void SetActiveGuiLayoutMetrics(const GuiLayoutMetrics& metrics) noexcept;

[[nodiscard]] float GetGlobalGuiUiScale() noexcept;
void SetGlobalGuiUiScale(float scale) noexcept;

/** GLFW / OS content scale (reported each frame; not applied to layout — see <c>GetEffectiveGuiUiScale</c>). */
[[nodiscard]] float GetSystemDpiScale() noexcept;
void RefreshSystemDpiScale(float contentScaleX, float contentScaleY) noexcept;

/**
 * User preference zoom (<c>ui_scale</c> in <c>editor_layout.ini</c>).
 * Layout runs in framebuffer pixels (already HiDPI-sized on Retina), so system content scale is not multiplied in.
 */
[[nodiscard]] float GetEffectiveGuiUiScale() noexcept;

}  // namespace Spark::Gui
