#include "spark/gui/toolkit/GuiToolkitSettings.hpp"

namespace Spark::Gui {

GuiToolkitKind GuiToolkitSettings::preferred = GuiToolkitKind::SparkNative;

GuiToolkitKind GuiToolkitSettings::GetPreferred() noexcept {
    return preferred;
}

void GuiToolkitSettings::SetPreferred(const GuiToolkitKind kind) noexcept {
    preferred = kind;
}

bool GuiToolkitSettings::ShouldProcessSparkGuiInput() noexcept {
    return preferred != GuiToolkitKind::DearImGui;
}

}  // namespace Spark::Gui
