#pragma once

#include "spark/ui/core/UiThemeCatalog.hpp"

namespace Spark::Ui {

/** Persisted scene-editor chrome (sidebar width, scroll). */
struct SceneEditorLayoutSettings {
    float sidebarSplit = 0.31F;
    float sidebarWidthPx = 600.0F;
    float inspectorScrollY = 0.0F;
    float uiScale = 1.0F;
    UiThemePreset guiTheme = UiThemePreset::ClassicMint;
    bool sidebarSplitFromFile = false;
};

[[nodiscard]] float GetSceneEditorSidebarSplit() noexcept;
void SetSceneEditorSidebarSplit(float split) noexcept;

[[nodiscard]] float GetSceneEditorSidebarWidthPx() noexcept;
void SetSceneEditorSidebarWidthPx(float widthPx) noexcept;

[[nodiscard]] bool TryLoadSceneEditorLayout(SceneEditorLayoutSettings& out) noexcept;
[[nodiscard]] bool SaveSceneEditorLayout(const SceneEditorLayoutSettings& in) noexcept;

}  // namespace Spark::Ui
