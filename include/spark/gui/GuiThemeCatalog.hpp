#pragma once

#include "spark/gui/GuiTheme.hpp"

namespace Spark::Gui {

/** Built-in GUI palette presets selectable in the shell demo and persisted in <c>editor_layout.ini</c>. */
enum class GuiThemePreset : int {
    ClassicMint = 0,
    SceneEditorDark = 1,
    HighContrastLight = 2,
    HighContrastDark = 3,
    HighContrastYellowOnBlack = 4,
    TwilightSlate = 5,
};

[[nodiscard]] constexpr int GuiThemePresetCount() noexcept {
    return 6;
}

[[nodiscard]] GuiThemePreset GetActiveGuiThemePreset() noexcept;
void SetActiveGuiThemePreset(GuiThemePreset preset) noexcept;

[[nodiscard]] GuiTheme ResolveGuiTheme(GuiThemePreset preset) noexcept;
[[nodiscard]] const char* GetGuiThemePresetDisplayName(GuiThemePreset preset) noexcept;

/** Parses a preset id from layout file; unknown values map to <c>ClassicMint</c>. */
[[nodiscard]] GuiThemePreset GuiThemePresetFromId(int id) noexcept;

}  // namespace Spark::Gui
