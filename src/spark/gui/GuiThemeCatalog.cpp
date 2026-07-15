#include "spark/gui/GuiThemeCatalog.hpp"

namespace Spark::Gui {

namespace {

GuiThemePreset gActivePreset = GuiThemePreset::ClassicMint;

}  // namespace

GuiThemePreset GetActiveGuiThemePreset() noexcept {
    return gActivePreset;
}

void SetActiveGuiThemePreset(const GuiThemePreset preset) noexcept {
    gActivePreset = preset;
}

GuiTheme ResolveGuiTheme(const GuiThemePreset preset) noexcept {
    switch (preset) {
    case GuiThemePreset::SceneEditorDark:
        return GuiTheme::SceneEditorDark();
    case GuiThemePreset::HighContrastLight:
        return GuiTheme::HighContrastLight();
    case GuiThemePreset::HighContrastDark:
        return GuiTheme::HighContrastDark();
    case GuiThemePreset::HighContrastYellowOnBlack:
        return GuiTheme::HighContrastYellowOnBlack();
    case GuiThemePreset::ClassicMint:
    default:
        return GuiTheme::ClassicMint();
    }
}

const char* GetGuiThemePresetDisplayName(const GuiThemePreset preset) noexcept {
    switch (preset) {
    case GuiThemePreset::SceneEditorDark:
        return "Scene editor dark";
    case GuiThemePreset::HighContrastLight:
        return "High contrast light";
    case GuiThemePreset::HighContrastDark:
        return "High contrast dark";
    case GuiThemePreset::HighContrastYellowOnBlack:
        return "High contrast yellow on black";
    case GuiThemePreset::ClassicMint:
    default:
        return "Classic mint";
    }
}

GuiThemePreset GuiThemePresetFromId(const int id) noexcept {
    switch (id) {
    case 1:
        return GuiThemePreset::SceneEditorDark;
    case 2:
        return GuiThemePreset::HighContrastLight;
    case 3:
        return GuiThemePreset::HighContrastDark;
    case 4:
        return GuiThemePreset::HighContrastYellowOnBlack;
    case 0:
    default:
        return GuiThemePreset::ClassicMint;
    }
}

}  // namespace Spark::Gui
