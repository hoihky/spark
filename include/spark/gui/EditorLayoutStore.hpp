#pragma once

#include "spark/gui/GuiThemeCatalog.hpp"

namespace Spark::Gui {

/** Persisted scene-editor chrome (sidebar width, scroll). */
struct SceneEditorLayoutSettings {
    /** Splitter fraction for the left pane (0.08–0.92); preferred over pixel width when loading. */
    float sidebarSplit = 0.31F;
    float sidebarWidthPx = 600.0F;
    float inspectorScrollY = 0.0F;
    float uiScale = 1.0F;
    GuiThemePreset guiTheme = GuiThemePreset::ClassicMint;
    /** True when <c>sidebar_split=</c> was present in the layout file. */
    bool sidebarSplitFromFile = false;
};

[[nodiscard]] float GetSceneEditorSidebarSplit() noexcept;
void SetSceneEditorSidebarSplit(float split) noexcept;

[[nodiscard]] float GetSceneEditorSidebarWidthPx() noexcept;
void SetSceneEditorSidebarWidthPx(float widthPx) noexcept;

[[nodiscard]] bool TryLoadSceneEditorLayout(SceneEditorLayoutSettings& out) noexcept;
[[nodiscard]] bool SaveSceneEditorLayout(const SceneEditorLayoutSettings& in) noexcept;

}  // namespace Spark::Gui
