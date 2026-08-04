#pragma once

namespace Spark::Ui {

/**
 * Scalable layout constants for retained-mode UI (padding, row heights, font sizes).
 * Base values are defined at <c>uiScale == 1</c>; multiply by <c>uiScale</c> for DPI / editor zoom.
 */
struct UiLayoutMetrics {
    float uiScale = 1.0F;

    float panelPadding = 8.0F;
    float groupPadding = 10.0F;
    float formRowHeight = 28.0F;
    float listRowHeight = 30.0F;
    float controlGap = 4.0F;
    float fontBody = 18.0F;
    float fontControl = 22.0F;
    float fontLabel = 22.0F;
    float fontSmall = 15.0F;

    [[nodiscard]] float Scaled(const float base) const noexcept { return base * uiScale; }

    [[nodiscard]] float Padding() const noexcept { return Scaled(panelPadding); }
    [[nodiscard]] float GroupPadding() const noexcept { return Scaled(groupPadding); }
    [[nodiscard]] float FormRowHeight() const noexcept { return Scaled(formRowHeight); }
    [[nodiscard]] float ListRowHeight() const noexcept { return Scaled(listRowHeight); }
    [[nodiscard]] float ControlGap() const noexcept { return Scaled(controlGap); }
    [[nodiscard]] float FontBody() const noexcept { return Scaled(fontBody); }
    [[nodiscard]] float FontControl() const noexcept { return Scaled(fontControl); }
    [[nodiscard]] float FontLabel() const noexcept { return Scaled(fontLabel); }
    [[nodiscard]] float FontSmall() const noexcept { return Scaled(fontSmall); }

    [[nodiscard]] static UiLayoutMetrics Default() noexcept;
};

[[nodiscard]] const UiLayoutMetrics& GetActiveUiLayoutMetrics() noexcept;
void SetActiveUiLayoutMetrics(const UiLayoutMetrics& metrics) noexcept;

[[nodiscard]] float GetGlobalUiScale() noexcept;
void SetGlobalUiScale(float scale) noexcept;

}  // namespace Spark::Ui
