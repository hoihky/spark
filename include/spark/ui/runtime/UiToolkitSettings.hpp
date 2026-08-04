#pragma once

#include "spark/ui/runtime/UiBackendKind.hpp"

namespace Spark::Ui {

/**
 * Selects the active <c>spark/ui</c> backend (Spark native vs Dear ImGui retained controls).
 */
class UiToolkitSettings {
public:
    [[nodiscard]] static UiBackendKind GetPreferred() noexcept;
    static void SetPreferred(UiBackendKind kind) noexcept;

    /** When Dear ImGui is active, retained Spark controls skip router hit-testing (ImGui owns input). */
    [[nodiscard]] static bool ShouldProcessSparkUiInput() noexcept;

private:
    static UiBackendKind preferred;
};

}  // namespace Spark::Ui
