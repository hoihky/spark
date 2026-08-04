#include "spark/ui/core/UiThemeCatalog.hpp"

namespace Spark::Ui {

namespace {

UiThemePreset gActivePreset = UiThemePreset::ClassicMint;

}  // namespace

UiThemePreset GetActiveUiThemePreset() noexcept {
    return gActivePreset;
}

void SetActiveUiThemePreset(const UiThemePreset preset) noexcept {
    gActivePreset = preset;
}

UiTheme ResolveUiTheme(const UiThemePreset preset) noexcept {
    switch (preset) {
    case UiThemePreset::SceneEditorDark:
        return UiTheme::SceneEditorDark();
    case UiThemePreset::HighContrastLight:
        return UiTheme::HighContrastLight();
    case UiThemePreset::HighContrastDark:
        return UiTheme::HighContrastDark();
    case UiThemePreset::HighContrastYellowOnBlack:
        return UiTheme::HighContrastYellowOnBlack();
    case UiThemePreset::TwilightSlate:
        return UiTheme::TwilightSlate();
    case UiThemePreset::ClassicMint:
    default:
        return UiTheme::ClassicMint();
    }
}

const char* GetUiThemePresetDisplayName(const UiThemePreset preset) noexcept {
    switch (preset) {
    case UiThemePreset::SceneEditorDark:
        return "Scene editor dark";
    case UiThemePreset::HighContrastLight:
        return "High contrast light";
    case UiThemePreset::HighContrastDark:
        return "High contrast dark";
    case UiThemePreset::HighContrastYellowOnBlack:
        return "High contrast yellow on black";
    case UiThemePreset::TwilightSlate:
        return "Twilight slate";
    case UiThemePreset::ClassicMint:
    default:
        return "Classic mint";
    }
}

UiThemePreset UiThemePresetFromId(const int id) noexcept {
    switch (id) {
    case 1:
        return UiThemePreset::SceneEditorDark;
    case 2:
        return UiThemePreset::HighContrastLight;
    case 3:
        return UiThemePreset::HighContrastDark;
    case 4:
        return UiThemePreset::HighContrastYellowOnBlack;
    case 5:
        return UiThemePreset::TwilightSlate;
    case 0:
    default:
        return UiThemePreset::ClassicMint;
    }
}

}  // namespace Spark::Ui
