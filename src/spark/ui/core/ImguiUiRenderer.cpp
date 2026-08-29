#include "spark/ui/core/ImguiUiRenderer.hpp"

#include "spark/config.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/imgui/IImGuiLayer.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/ui/runtime/UiFrameContext.hpp"

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstdio>
#endif

namespace Spark::Ui {

namespace {

#if SPARK_ENABLE_IMGUI
void FormatPanelWindowName(const char* id, Utf8StringView title, char* out, const std::size_t outSize) {
    if (title.IsEmpty() && id != nullptr) {
        std::snprintf(out, outSize, "%s", id);
    } else if (id != nullptr) {
        std::snprintf(out, outSize, "%s###%s", title.CStr(), id);
    } else {
        std::snprintf(out, outSize, "%s", title.CStr());
    }
}

[[nodiscard]] bool ImGuiLayerReady(const IImGuiLayer* layer) noexcept {
    return layer != nullptr && layer->IsEnabled();
}
#endif

}  // namespace

ImguiUiRenderer::ImguiUiRenderer(IImGuiLayer* layer) noexcept : imguiLayer(layer) {
    themePtr = &fallbackTheme;
    metricsPtr = &fallbackMetrics;
}

void ImguiUiRenderer::SetImGuiLayer(IImGuiLayer* layer) noexcept {
    imguiLayer = layer;
}

void ImguiUiRenderer::BeginBackendFrame(Window& window, IInput& input, const UiFrameContext& context) {
    panelStack = 0;
    scrollStack = 0;
    activeScrollY = 0.0F;
    if (!ImGuiLayerReady(imguiLayer)) {
        return;
    }
    ImGuiFrameTiming timing{};
    timing.deltaTimeSeconds = context.deltaTimeSeconds;
    imguiLayer->BeginFrame(window, input, timing);
}

void ImguiUiRenderer::EndBackendFrame() {
    if (!ImGuiLayerReady(imguiLayer)) {
        return;
    }
    imguiLayer->EndFrame();
}

void ImguiUiRenderer::SetTheme(const UiTheme* themeIn) noexcept {
    themePtr = themeIn != nullptr ? themeIn : &fallbackTheme;
}

const UiTheme& ImguiUiRenderer::GetTheme() const noexcept {
    return themePtr != nullptr ? *themePtr : fallbackTheme;
}

void ImguiUiRenderer::SetLayoutFont(const Font* /*font*/) noexcept {}

void ImguiUiRenderer::SetLayoutMetrics(const UiLayoutMetrics* metricsIn) noexcept {
    metricsPtr = metricsIn != nullptr ? metricsIn : &fallbackMetrics;
}

const UiLayoutMetrics& ImguiUiRenderer::GetLayoutMetrics() const noexcept {
    return metricsPtr != nullptr ? *metricsPtr : fallbackMetrics;
}

void ImguiUiRenderer::SetInteraction(IUiElement* /*hot*/, IUiElement* /*active*/, IUiElement* /*focus*/) noexcept {}

bool ImguiUiRenderer::IsHot(const IUiElement* /*element*/) const noexcept {
    return false;
}

bool ImguiUiRenderer::IsActive(const IUiElement* /*element*/) const noexcept {
    return false;
}

bool ImguiUiRenderer::IsFocused(const IUiElement* /*element*/) const noexcept {
    return false;
}

void ImguiUiRenderer::FillRect(
        const float /*x*/,
        const float /*y*/,
        const float /*w*/,
        const float /*h*/,
        const Vector3& /*rgb*/,
        const float /*alpha*/,
        const SceneBlendMode /*blend*/) {}

void ImguiUiRenderer::FillRectGradientVertical(
        const float /*x*/,
        const float /*y*/,
        const float /*w*/,
        const float /*h*/,
        const Vector3& /*rgbTop*/,
        const Vector3& /*rgbBottom*/,
        const float /*alpha*/,
        const SceneBlendMode /*blend*/) {}

void ImguiUiRenderer::FillDropShadow(
        const float /*x*/,
        const float /*y*/,
        const float /*w*/,
        const float /*h*/,
        const float /*offsetX*/,
        const float /*offsetY*/,
        const Vector3& /*rgb*/,
        const float /*alpha*/) {}

void ImguiUiRenderer::FillRoundRectGradientVertical(
        const float /*x*/,
        const float /*y*/,
        const float /*w*/,
        const float /*h*/,
        const float /*cornerRadius*/,
        const Vector3& /*rgbTop*/,
        const Vector3& /*rgbBottom*/,
        const float /*alpha*/,
        const SceneBlendMode /*blend*/) {}

void ImguiUiRenderer::StrokeRect(
        const float /*x*/,
        const float /*y*/,
        const float /*w*/,
        const float /*h*/,
        const float /*strokeWidth*/,
        const Vector3& /*rgb*/,
        const float /*alpha*/) {}

void ImguiUiRenderer::StrokeRoundRect(
        const float /*x*/,
        const float /*y*/,
        const float /*w*/,
        const float /*h*/,
        const float /*cornerRadius*/,
        const float /*strokeWidth*/,
        const Vector3& /*rgb*/,
        const float /*alpha*/) {}

void ImguiUiRenderer::DrawText(
        const float /*x*/,
        const float /*y*/,
        const float /*maxWidth*/,
        const Utf8StringView /*text*/,
        const Vector3& /*rgb*/,
        const float /*alpha*/,
        const float /*fontSizePx*/,
        const bool /*bold*/) {}

void ImguiUiRenderer::DrawTextInRect(
        const Rect& /*rect*/,
        const float /*fontSizePx*/,
        const Utf8StringView /*text*/,
        const Vector3& /*rgb*/,
        const float /*alpha*/,
        const bool /*bold*/,
        const TextLayout /*layout*/) {}

Utf8String ImguiUiRenderer::EllipsizeUtf8(const Utf8StringView text, const float /*fontSizePx*/, const float /*maxWidth*/) const {
    return Utf8String(text.CStr());
}

void ImguiUiRenderer::PushClip(const Rect& /*rect*/) {}
void ImguiUiRenderer::PopClip() {}
void ImguiUiRenderer::PushOverlayLayer() {}
void ImguiUiRenderer::PushLateLayer() {}

bool ImguiUiRenderer::WantsCaptureMouse() const noexcept {
    return imguiLayer != nullptr && imguiLayer->IsEnabled() && imguiLayer->WantsCaptureMouse();
}

bool ImguiUiRenderer::WantsCaptureKeyboard() const noexcept {
    return imguiLayer != nullptr && imguiLayer->IsEnabled() && imguiLayer->WantsCaptureKeyboard();
}

void ImguiUiRenderer::TextUnformatted(const Utf8StringView text) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return;
    }
    ImGui::TextUnformatted(text.CStr());
#else
    (void)text;
#endif
}

void ImguiUiRenderer::TextDisabled(const Utf8StringView text) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return;
    }
    ImGui::TextDisabled("%s", text.CStr());
#else
    (void)text;
#endif
}

void ImguiUiRenderer::Separator() {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return;
    }
    ImGui::Separator();
#endif
}

bool ImguiUiRenderer::Button(const char* id, const Utf8StringView label) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return false;
    }
    ImGui::PushID(id);
    const bool pressed = ImGui::Button(label.CStr());
    ImGui::PopID();
    return pressed;
#else
    (void)id;
    (void)label;
    return false;
#endif
}

bool ImguiUiRenderer::Checkbox(const char* id, const Utf8StringView label, bool& value) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return false;
    }
    ImGui::PushID(id);
    const bool changed = ImGui::Checkbox(label.CStr(), &value);
    ImGui::PopID();
    return changed;
#else
    (void)id;
    (void)label;
    (void)value;
    return false;
#endif
}

bool ImguiUiRenderer::SliderFloat(
        const char* id,
        const Utf8StringView label,
        float& value,
        const float minValue,
        const float maxValue) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return false;
    }
    ImGui::PushID(id);
    const bool changed = ImGui::SliderFloat(label.CStr(), &value, minValue, maxValue);
    ImGui::PopID();
    return changed;
#else
    (void)id;
    (void)label;
    (void)value;
    (void)minValue;
    (void)maxValue;
    return false;
#endif
}

bool ImguiUiRenderer::DragFloat(
        const char* id,
        const Utf8StringView label,
        float& value,
        const float speed,
        const float minValue,
        const float maxValue) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return false;
    }
    ImGui::PushID(id);
    const bool changed = ImGui::DragFloat(label.CStr(), &value, speed, minValue, maxValue);
    ImGui::PopID();
    return changed;
#else
    (void)id;
    (void)label;
    (void)value;
    (void)speed;
    (void)minValue;
    (void)maxValue;
    return false;
#endif
}

void ImguiUiRenderer::SameLine(const float offsetFromStartX, const float spacing) {
#if SPARK_ENABLE_IMGUI
    if (ImGuiLayerReady(imguiLayer)) {
        ImGui::SameLine(offsetFromStartX, spacing);
    }
#else
    (void)offsetFromStartX;
    (void)spacing;
#endif
}

bool ImguiUiRenderer::IsAnyItemActive() const noexcept {
#if SPARK_ENABLE_IMGUI
    return ImGuiLayerReady(imguiLayer) && ImGui::IsAnyItemActive();
#else
    return false;
#endif
}

bool ImguiUiRenderer::BeginPanel(
        const char* id,
        const Utf8StringView title,
        bool* open,
        const Rect& bounds,
        const ImguiPanelPlacement placement) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return false;
    }
    if (open != nullptr && !*open) {
        return false;
    }
    char windowName[192];
    FormatPanelWindowName(id, title, windowName, sizeof(windowName));

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 vpPos = viewport != nullptr ? viewport->WorkPos : ImVec2(0.0F, 0.0F);
    const ImVec2 vpSize =
            viewport != nullptr ? viewport->WorkSize : ImGui::GetIO().DisplaySize;

    float useW = bounds.width > 0.0F ? bounds.width : 360.0F;
    float useH = bounds.height > 0.0F ? bounds.height : 280.0F;
    float useX = bounds.x;
    float useY = bounds.y;
    if (placement == ImguiPanelPlacement::CenterOnce) {
        useX = (vpSize.x - useW) * 0.5F;
        useY = (vpSize.y - useH) * 0.5F;
    }

    ImGuiCond posCond = ImGuiCond_FirstUseEver;
    ImGuiCond sizeCond = ImGuiCond_FirstUseEver;
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
    const bool lockedPlacement =
            placement == ImguiPanelPlacement::LockedSide || placement == ImguiPanelPlacement::LockedViewport;
    const bool viewportPlacement =
            placement == ImguiPanelPlacement::DockedPassthrough || placement == ImguiPanelPlacement::LockedViewport;
    if (viewportPlacement) {
        flags |= ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;
    } else if (placement != ImguiPanelPlacement::Docked) {
        flags |= ImGuiWindowFlags_NoDocking;
    }
    if (lockedPlacement) {
        posCond = ImGuiCond_Always;
        sizeCond = ImGuiCond_Always;
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
    }

    if (placement != ImguiPanelPlacement::Docked && placement != ImguiPanelPlacement::DockedPassthrough) {
        ImGui::SetNextWindowPos(ImVec2(vpPos.x + useX, vpPos.y + useY), posCond);
        ImGui::SetNextWindowSize(ImVec2(useW, useH), sizeCond);
    }
    if (nextPanelDockId != 0U &&
        (placement == ImguiPanelPlacement::Docked || placement == ImguiPanelPlacement::DockedPassthrough)) {
        ImGui::SetNextWindowDockID(static_cast<ImGuiID>(nextPanelDockId), ImGuiCond_FirstUseEver);
        nextPanelDockId = 0U;
    }
    ImGui::SetNextWindowViewport(viewport != nullptr ? viewport->ID : 0);

    (void)ImGui::Begin(windowName, open, flags);
    ++panelStack;
    return true;
#else
    (void)id;
    (void)title;
    (void)open;
    (void)bounds;
    (void)placement;
    return false;
#endif
}

void ImguiUiRenderer::EndPanel() {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return;
    }
    if (panelStack > 0) {
        ImGui::End();
        --panelStack;
    }
#endif
}

bool ImguiUiRenderer::BeginScrollRegion(const char* id, const float height) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return false;
    }
    const ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar;
    const float childHeight = height > 0.0F ? height : -1.0F;
    (void)ImGui::BeginChild(id, ImVec2(0.0F, childHeight), ImGuiChildFlags_None, flags);
    ++scrollStack;
    return true;
#else
    (void)id;
    (void)height;
    return false;
#endif
}

void ImguiUiRenderer::EndScrollRegion() {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return;
    }
    if (scrollStack > 0) {
        activeScrollY = ImGui::GetScrollY();
        ImGui::EndChild();
        --scrollStack;
    }
#endif
}

void ImguiUiRenderer::SetScrollY(const float y) {
#if SPARK_ENABLE_IMGUI
    if (scrollStack > 0) {
        ImGui::SetScrollY(y);
    }
    activeScrollY = y;
#else
    (void)y;
#endif
}

float ImguiUiRenderer::GetScrollY() const noexcept {
    return activeScrollY;
}

bool ImguiUiRenderer::BeginDockWorkspace(
        const char* id,
        const Rect& bounds,
        const float leftWidth,
        const float rightWidth,
        const char* leftWindowName,
        const char* centerWindowName,
        const char* rightWindowName,
        bool& layoutBuilt) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return false;
    }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 vpPos = viewport != nullptr ? viewport->WorkPos : ImVec2(0.0F, 0.0F);
    ImGui::SetNextWindowPos(ImVec2(vpPos.x + bounds.x, vpPos.y + bounds.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(bounds.width, bounds.height), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                             ImGuiWindowFlags_NoDocking;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
    if (!ImGui::Begin(id, nullptr, flags)) {
        ImGui::PopStyleVar(3);
        return false;
    }
    const ImGuiID dockSpaceId = ImGui::GetID("SparkUiDockSpace");
    ImGui::DockSpace(dockSpaceId, ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_PassthruCentralNode);
    if (!layoutBuilt) {
        ImGui::DockBuilderRemoveNode(dockSpaceId);
        ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockSpaceId, ImVec2(bounds.width, bounds.height));
        ImGuiID mainNode = dockSpaceId;
        const float safeWidth = (std::max)(bounds.width, 1.0F);
        const float leftRatio = std::clamp(leftWidth / safeWidth, 0.12F, 0.42F);
        const float rightRatio = std::clamp(rightWidth / safeWidth, 0.12F, 0.42F);
        const float centerRatio = std::max(0.16F, 1.0F - leftRatio - rightRatio);
        const float adjustedRightRatio = rightRatio / (centerRatio + rightRatio);
        ImGuiID leftNode = 0;
        ImGuiID centerNode = mainNode;
        ImGuiID rightNode = 0;
        leftNode = ImGui::DockBuilderSplitNode(mainNode, ImGuiDir_Left, leftRatio, nullptr, &centerNode);
        rightNode = ImGui::DockBuilderSplitNode(centerNode, ImGuiDir_Right, adjustedRightRatio, nullptr, &centerNode);
        if (leftWindowName != nullptr) {
            ImGui::DockBuilderDockWindow(leftWindowName, leftNode);
        }
        if (centerWindowName != nullptr) {
            ImGui::DockBuilderDockWindow(centerWindowName, centerNode);
        }
        if (rightWindowName != nullptr) {
            ImGui::DockBuilderDockWindow(rightWindowName, rightNode);
        }
        ImGui::DockBuilderFinish(dockSpaceId);
        layoutBuilt = true;
    }
    ++panelStack;
    return true;
#else
    (void)id;
    (void)bounds;
    (void)leftWidth;
    (void)rightWidth;
    (void)leftWindowName;
    (void)centerWindowName;
    (void)rightWindowName;
    (void)layoutBuilt;
    return false;
#endif
}

void ImguiUiRenderer::EndDockWorkspace() {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return;
    }
    if (panelStack > 0) {
        ImGui::End();
        --panelStack;
    }
    ImGui::PopStyleVar(3);
#endif
}

bool ImguiUiRenderer::BeginDockHost(const char* id, const Rect& bounds, unsigned int& outDockSpaceId) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return false;
    }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 vpPos = viewport != nullptr ? viewport->WorkPos : ImVec2(0.0F, 0.0F);
    ImGui::SetNextWindowPos(ImVec2(vpPos.x + bounds.x, vpPos.y + bounds.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(bounds.width, bounds.height), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                             ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
    if (!ImGui::Begin(id, nullptr, flags)) {
        ImGui::PopStyleVar(3);
        return false;
    }
    outDockSpaceId = ImGui::GetID("SparkUiDockSpace");
    ImGui::DockSpace(outDockSpaceId, ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_PassthruCentralNode);
    ++panelStack;
    return true;
#else
    (void)id;
    (void)bounds;
    (void)outDockSpaceId;
    return false;
#endif
}

void ImguiUiRenderer::BuildEditorDockLayout(
        const unsigned int dockSpaceId,
        const Rect& bounds,
        const EditorDockLayoutDesc& desc,
        EditorDockLayoutNodes& outNodes) {
#if SPARK_ENABLE_IMGUI
    if (!ImGuiLayerReady(imguiLayer)) {
        return;
    }
    ImGui::DockBuilderRemoveNode(static_cast<ImGuiID>(dockSpaceId));
    ImGui::DockBuilderAddNode(static_cast<ImGuiID>(dockSpaceId), ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(static_cast<ImGuiID>(dockSpaceId), ImVec2(bounds.width, bounds.height));

    const float safeW = (std::max)(bounds.width, 1.0F);
    const float safeH = (std::max)(bounds.height, 1.0F);
    const float topRatio = std::clamp(desc.toolbarHeightPx / safeH, 0.03F, 0.12F);
    const float leftRatio = std::clamp(desc.leftWidthPx / safeW, 0.12F, 0.42F);
    const float rightRatio = std::clamp(desc.rightWidthPx / safeW, 0.12F, 0.42F);
    const float leftSplit = std::clamp(desc.leftStackSplit, 0.2F, 0.8F);

    ImGuiID rootNode = static_cast<ImGuiID>(dockSpaceId);
    ImGuiID topNode = 0;
    ImGuiID bodyNode = rootNode;
    topNode = ImGui::DockBuilderSplitNode(rootNode, ImGuiDir_Up, topRatio, &topNode, &bodyNode);

    ImGuiID leftNode = 0;
    ImGuiID centerRightNode = bodyNode;
    leftNode = ImGui::DockBuilderSplitNode(bodyNode, ImGuiDir_Left, leftRatio, &leftNode, &centerRightNode);

    ImGuiID hierarchyNode = 0;
    ImGuiID projectNode = leftNode;
    hierarchyNode = ImGui::DockBuilderSplitNode(leftNode, ImGuiDir_Up, leftSplit, &hierarchyNode, &projectNode);

    ImGuiID centerNode = centerRightNode;
    ImGuiID rightNode = 0;
    const float centerShare = std::max(0.16F, 1.0F - rightRatio);
    const float rightSplit = rightRatio / (centerShare + rightRatio);
    rightNode = ImGui::DockBuilderSplitNode(centerRightNode, ImGuiDir_Right, rightSplit, &rightNode, &centerNode);

    outNodes.toolbar = topNode;
    outNodes.hierarchy = hierarchyNode;
    outNodes.project = projectNode;
    outNodes.scene = centerNode;
    outNodes.inspector = rightNode;

    if (desc.toolbarWindow != nullptr) {
        ImGui::DockBuilderDockWindow(desc.toolbarWindow, topNode);
    }
    if (desc.hierarchyWindow != nullptr) {
        ImGui::DockBuilderDockWindow(desc.hierarchyWindow, hierarchyNode);
    }
    if (desc.projectWindow != nullptr) {
        ImGui::DockBuilderDockWindow(desc.projectWindow, projectNode);
    }
    if (desc.inspectorWindow != nullptr) {
        ImGui::DockBuilderDockWindow(desc.inspectorWindow, rightNode);
    }
    ImGui::DockBuilderFinish(static_cast<ImGuiID>(dockSpaceId));
#else
    (void)dockSpaceId;
    (void)bounds;
    (void)desc;
    (void)outNodes;
#endif
}

void ImguiUiRenderer::EndDockHost() {
    EndDockWorkspace();
}

}  // namespace Spark::Ui
