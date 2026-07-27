#include "spark/gui/toolkit/GuiToolkitSettings.hpp"

#include "spark/gui/api/GuiBackendKind.hpp"
#include "spark/gui/api/GuiSystem.hpp"

namespace Spark::Gui {

GuiToolkitKind GuiToolkitSettings::preferred = GuiToolkitKind::SparkNative;

GuiToolkitKind GuiToolkitSettings::GetPreferred() noexcept {
    return preferred;
}

void GuiToolkitSettings::SetPreferred(const GuiToolkitKind kind) noexcept {
    preferred = kind;
    GuiSystem::Get().SetActiveBackend(static_cast<GuiBackendKind>(kind));
}

bool GuiToolkitSettings::ShouldProcessSparkGuiInput() noexcept {
    return GetPreferred() == GuiToolkitKind::SparkNative;
}

}  // namespace Spark::Gui
