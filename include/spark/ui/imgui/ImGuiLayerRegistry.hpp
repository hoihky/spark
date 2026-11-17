#pragma once

namespace Spark {

class IImGuiLayer;

/** Global pointer set once by the engine host so GUI input routing can query ImGui capture state. */
void SetActiveImGuiLayer(IImGuiLayer* layer) noexcept;
[[nodiscard]] IImGuiLayer* GetActiveImGuiLayer() noexcept;

}  // namespace Spark
