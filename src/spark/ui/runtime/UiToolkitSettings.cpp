#include "spark/ui/runtime/UiToolkitSettings.hpp"

#include "spark/ui/runtime/UiSystem.hpp"

namespace Spark::Ui {

UiBackendKind UiToolkitSettings::preferred = UiBackendKind::SparkNative;

UiBackendKind UiToolkitSettings::GetPreferred() noexcept {
    return preferred;
}

void UiToolkitSettings::SetPreferred(const UiBackendKind kind) noexcept {
    preferred = kind;
    UiSystem::Get().SetActiveBackend(kind);
}

bool UiToolkitSettings::ShouldProcessSparkUiInput() noexcept {
    return GetPreferred() == UiBackendKind::SparkNative;
}

}  // namespace Spark::Ui
