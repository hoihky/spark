#pragma once

#include <cstdint>

namespace Spark::Ui {

/** Active UI implementation (Strategy). */
enum class UiBackendKind : std::uint8_t {
    /** Retained <c>IUiElement</c> trees painted via <c>SparkUiRenderer</c> / Vulkan screen UI pass. */
    SparkNative = 0,
    /** Dear ImGui retained controls (<c>SPARK_ENABLE_IMGUI</c>). */
    DearImGui = 1,
};

}  // namespace Spark::Ui
