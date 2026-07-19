#include "spark/imgui/ImGuiLayerRegistry.hpp"

namespace Spark {

namespace {
IImGuiLayer* gActiveLayer = nullptr;
}  // namespace

void SetActiveImGuiLayer(IImGuiLayer* const layer) noexcept {
    gActiveLayer = layer;
}

IImGuiLayer* GetActiveImGuiLayer() noexcept {
    return gActiveLayer;
}

}  // namespace Spark
