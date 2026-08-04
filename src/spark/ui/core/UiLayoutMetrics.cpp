#include "spark/ui/core/UiLayoutMetrics.hpp"

#include <algorithm>

namespace Spark::Ui {

namespace {

UiLayoutMetrics gActiveMetrics{};
float gGlobalUiScale = 1.0F;

[[nodiscard]] float ClampUiScale(const float scale) noexcept {
    return std::clamp(scale, 0.75F, 2.5F);
}

}  // namespace

UiLayoutMetrics UiLayoutMetrics::Default() noexcept {
    UiLayoutMetrics m{};
    m.uiScale = ClampUiScale(gGlobalUiScale);
    return m;
}

const UiLayoutMetrics& GetActiveUiLayoutMetrics() noexcept {
    return gActiveMetrics;
}

void SetActiveUiLayoutMetrics(const UiLayoutMetrics& metrics) noexcept {
    gActiveMetrics = metrics;
}

float GetGlobalUiScale() noexcept {
    return gGlobalUiScale;
}

void SetGlobalUiScale(const float scale) noexcept {
    gGlobalUiScale = ClampUiScale(scale);
    gActiveMetrics.uiScale = gGlobalUiScale;
}

}  // namespace Spark::Ui
