#pragma once

#include "spark/ui/core/UiTheme.hpp"

namespace Spark::Ui {

/** Built-in UI palette presets selectable in the shell demo and persisted in <c>editor_layout.ini</c>. */
enum class UiThemePreset : int {
    ClassicMint = 0,
    SceneEditorDark = 1,
    HighContrastLight = 2,
    HighContrastDark = 3,
    HighContrastYellowOnBlack = 4,
    TwilightSlate = 5,
};

[[nodiscard]] constexpr int UiThemePresetCount() noexcept {
    return 6;
}

[[nodiscard]] UiThemePreset GetActiveUiThemePreset() noexcept;
void SetActiveUiThemePreset(UiThemePreset preset) noexcept;

[[nodiscard]] UiTheme ResolveUiTheme(UiThemePreset preset) noexcept;
[[nodiscard]] const char* GetUiThemePresetDisplayName(UiThemePreset preset) noexcept;

/** Parses a preset id from layout file; unknown values map to <c>ClassicMint</c>. */
[[nodiscard]] UiThemePreset UiThemePresetFromId(int id) noexcept;

}  // namespace Spark::Ui
