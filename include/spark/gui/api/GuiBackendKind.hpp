#pragma once

#include <cstdint>

namespace Spark::Gui {

/** Active GUI implementation (Strategy). */
enum class GuiBackendKind : std::uint8_t {
    /** Retained <c>Widget</c> trees + optional portable immediate API on <c>SceneRenderParams</c>. */
    SparkNative = 0,
    /** Dear ImGui immediate-mode toolkit (<c>SPARK_ENABLE_IMGUI</c>). */
    DearImGui = 1,
};

}  // namespace Spark::Gui
