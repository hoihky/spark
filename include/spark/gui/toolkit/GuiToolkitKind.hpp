#pragma once

#include <cstdint>

namespace Spark::Gui {

/** Which immediate/retained UI stack the running game prefers for new UI work. */
enum class GuiToolkitKind : std::uint8_t {
    /** Retained-mode <c>spark/gui</c> widgets via <c>GuiCanvasComponent</c>. */
    SparkNative = 0,
    /** Dear ImGui immediate-mode toolkit (requires <c>SPARK_ENABLE_IMGUI</c>). */
    DearImGui = 1,
};

}  // namespace Spark::Gui
