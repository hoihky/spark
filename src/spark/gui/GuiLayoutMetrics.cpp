#include "spark/gui/GuiLayoutMetrics.hpp"

#include <algorithm>

namespace Spark::Gui {

namespace {

GuiLayoutMetrics gActiveMetrics{};
float gGlobalUiScale = 1.0F;
float gSystemDpiScale = 1.0F;

[[nodiscard]] float ClampUiScale(const float scale) noexcept {
    return std::clamp(scale, 0.75F, 2.5F);
}

}  // namespace

GuiLayoutMetrics GuiLayoutMetrics::Default() noexcept {
    GuiLayoutMetrics m{};
    m.uiScale = ClampUiScale(gGlobalUiScale);
    return m;
}

const GuiLayoutMetrics& GetActiveGuiLayoutMetrics() noexcept {
    return gActiveMetrics;
}

void SetActiveGuiLayoutMetrics(const GuiLayoutMetrics& metrics) noexcept {
    gActiveMetrics = metrics;
    gActiveMetrics.uiScale = ClampUiScale(gGlobalUiScale);
}

float GetGlobalGuiUiScale() noexcept {
    return gGlobalUiScale;
}

void SetGlobalGuiUiScale(const float scale) noexcept {
    gGlobalUiScale = ClampUiScale(scale);
}

float GetSystemDpiScale() noexcept {
    return gSystemDpiScale;
}

void RefreshSystemDpiScale(const float contentScaleX, const float contentScaleY) noexcept {
    const float sx = contentScaleX > 0.01F ? contentScaleX : 1.0F;
    const float sy = contentScaleY > 0.01F ? contentScaleY : 1.0F;
    gSystemDpiScale = std::max(sx, sy);
}

float GetEffectiveGuiUiScale() noexcept {
    return ClampUiScale(gGlobalUiScale);
}

}  // namespace Spark::Gui
