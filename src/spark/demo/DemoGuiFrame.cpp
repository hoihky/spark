#include "spark/demo/DemoGuiFrame.hpp"

#include "spark/config.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/ui/runtime/UiToolkitSettings.hpp"

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace Spark::DemoGui {

namespace {

bool gDearImGuiStyleScaled = false;

}  // namespace

void ActivateDearImGuiDemoUi(IEngineContext& context) {
    Ui::UiToolkitSettings::SetPreferred(Ui::UiBackendKind::DearImGui);
    if (IImGuiLayer* layer = context.TryGetImGuiLayer()) {
        layer->SetEnabled(layer->IsAvailable());
    }
#if SPARK_ENABLE_IMGUI
    if (!gDearImGuiStyleScaled) {
        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleMain = kDearImGuiDemoUiScale;
        style.ScaleAllSizes(kDearImGuiDemoUiScale);
        gDearImGuiStyleScaled = true;
    }
#else
    (void)context;
#endif
}

void ShutdownDearImGuiDemoUi(IEngineContext& context) noexcept {
    if (IImGuiLayer* layer = context.TryGetImGuiLayer()) {
        layer->SetEnabled(false);
    }
#if SPARK_ENABLE_IMGUI
    if (gDearImGuiStyleScaled) {
        const float inv = 1.0F / kDearImGuiDemoUiScale;
        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleMain = 1.0F;
        style.ScaleAllSizes(inv);
        gDearImGuiStyleScaled = false;
    }
#else
    (void)context;
#endif
}

}  // namespace Spark::DemoGui
