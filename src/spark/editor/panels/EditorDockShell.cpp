#include "spark/editor/panels/EditorDockShell.hpp"

#include "spark/editor/EditorDefaultLayout.hpp"
#include "spark/editor/EditorViewport.hpp"
#include "spark/scene/editor/TransformGizmo.hpp"
#include "spark/ui/runtime/EditorLayoutStore.hpp"
#include "spark/ui/Ui.hpp"
#include "spark/ui/core/ImguiUiRenderer.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/spark/UiChild.hpp"

#include <algorithm>
#include <cstdio>

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#include <imgui_internal.h>
#endif

namespace Spark::Editor {

namespace {

void FormatImGuiWindowName(const char* id, const char* title, char* out, const std::size_t outSize) noexcept {
    if (title == nullptr || title[0] == '\0') {
        std::snprintf(out, outSize, "%s", id);
    } else {
        std::snprintf(out, outSize, "%s###%s", title, id);
    }
}

#if SPARK_ENABLE_IMGUI
/** ImGui layout uses display coordinates; Vulkan scissor and input use framebuffer pixels. */
Ui::Rect DisplayRectToFramebuffer(const Ui::Rect& displayRect) noexcept {
    const ImGuiIO& io = ImGui::GetIO();
    const float scaleX = io.DisplayFramebufferScale.x > 1.0e-4F ? io.DisplayFramebufferScale.x : 1.0F;
    const float scaleY = io.DisplayFramebufferScale.y > 1.0e-4F ? io.DisplayFramebufferScale.y : 1.0F;
    return Ui::Rect{
            displayRect.x * scaleX,
            displayRect.y * scaleY,
            displayRect.width * scaleX,
            displayRect.height * scaleY};
}
#endif

class EditorImGuiDockHost final : public Ui::UiElementBase {
public:
    EditorImGuiDockHost() : Ui::UiElementBase(Utf8String("editor_dock_root")) {}

    void Configure(const float toolbarH, const float leftW, const float rightW, const float leftSplit) noexcept {
        toolbarHeightPx = toolbarH;
        leftWidthPx = leftW;
        rightWidthPx = rightW;
        leftStackSplit = leftSplit;
        layoutBuilt = false;
    }

    [[nodiscard]] Ui::Rect GetCenterBounds() const noexcept { return centerBounds; }
    void InvalidateLayout() noexcept { layoutBuilt = false; }
    void RefreshWorldViewportBounds() noexcept { UpdateSceneViewportBounds(); }

protected:
    void DoMeasure(const Ui::UiMeasureConstraints& constraints, Ui::UiSize& outDesired) override {
        outDesired.width = constraints.maxWidth;
        outDesired.height = constraints.maxHeight;
    }

    void Arrange(const Ui::Rect& finalBounds) override {
        bounds = finalBounds;
        const float safeW = (std::max)(finalBounds.width, 1.0F);
        const float safeH = (std::max)(finalBounds.height, 1.0F);
        const float topH = std::min(toolbarHeightPx, safeH * 0.12F);
        const float bodyH = std::max(0.0F, safeH - topH);
        const float leftW = std::min(leftWidthPx, safeW * 0.42F);
        const float rightW = std::min(rightWidthPx, safeW * 0.42F);
        const float centerW = std::max(0.0F, safeW - leftW - rightW);
        centerBounds = Ui::Rect{finalBounds.x + leftW, finalBounds.y + topH, centerW, bodyH};
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i] != nullptr) {
                children[i]->Arrange(finalBounds);
            }
        }
    }

    void Paint(Ui::IUiRenderer& renderer) override {
        if (!visible) {
            return;
        }
        auto* imgui = dynamic_cast<Ui::ImguiUiRenderer*>(&renderer);
        if (imgui == nullptr) {
            UiElementBase::Paint(renderer);
            return;
        }

        unsigned int dockSpaceId = 0U;
        if (!imgui->BeginDockHost(GetId().CStr(), GetBounds(), dockSpaceId)) {
            return;
        }

        if (!layoutBuilt) {
            Ui::EditorDockLayoutDesc layoutDesc{};
            layoutDesc.toolbarHeightPx = toolbarHeightPx;
            layoutDesc.leftWidthPx = leftWidthPx;
            layoutDesc.rightWidthPx = rightWidthPx;
            layoutDesc.leftStackSplit = leftStackSplit;
            layoutDesc.toolbarWindow = toolbarWindowName;
            layoutDesc.hierarchyWindow = hierarchyWindowName;
            layoutDesc.projectWindow = projectWindowName;
            layoutDesc.inspectorWindow = inspectorWindowName;
            imgui->BuildEditorDockLayout(dockSpaceId, GetBounds(), layoutDesc, layoutNodes);
            layoutBuilt = true;
        }

        imgui->EndDockHost();

        const unsigned int childDockIds[] = {
                layoutNodes.toolbar,
                layoutNodes.hierarchy,
                layoutNodes.project,
                layoutNodes.inspector,
        };
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i] == nullptr) {
                continue;
            }
            if (i < sizeof(childDockIds) / sizeof(childDockIds[0]) && childDockIds[i] != 0U) {
                imgui->SetNextPanelDockId(childDockIds[i]);
            }
            children[i]->Paint(renderer);
        }

        UpdateSceneViewportBounds();
    }

    void UpdateSceneViewportBounds() noexcept {
#if SPARK_ENABLE_IMGUI
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const float originX = viewport != nullptr ? viewport->WorkPos.x : 0.0F;
        const float originY = viewport != nullptr ? viewport->WorkPos.y : 0.0F;

        Ui::Rect displayBounds = centerBounds;
        if (layoutNodes.scene != 0U) {
            ImGuiContext* imguiContext = ImGui::GetCurrentContext();
            ImGuiDockNode* centerNode = imguiContext != nullptr
                    ? ImGui::DockContextFindNodeByID(imguiContext, static_cast<ImGuiID>(layoutNodes.scene))
                    : ImGui::DockBuilderGetNode(static_cast<ImGuiID>(layoutNodes.scene));
            if (centerNode != nullptr && centerNode->IsVisible) {
                const ImRect rect = centerNode->Rect();
                if (rect.GetWidth() > 1.0F && rect.GetHeight() > 1.0F) {
                    displayBounds =
                            Ui::Rect{rect.Min.x - originX, rect.Min.y - originY, rect.GetWidth(), rect.GetHeight()};
                }
            }
        }
        centerBounds = DisplayRectToFramebuffer(displayBounds);
#else
        (void)layoutNodes;
#endif
    }

    void DoPaint(Ui::IUiRenderer& /*renderer*/) override {}

private:
    float toolbarHeightPx = EditorDefaultLayout::kToolbarHeightPx;
    float leftWidthPx = EditorDefaultLayout::kLeftWidthPx;
    float rightWidthPx = EditorDefaultLayout::kRightWidthPx;
    float leftStackSplit = EditorDefaultLayout::kLeftStackSplit;
    Ui::Rect centerBounds{};
    bool layoutBuilt = false;
    Ui::EditorDockLayoutNodes layoutNodes{};
    char toolbarWindowName[192]{};
    char hierarchyWindowName[192]{};
    char projectWindowName[192]{};
    char inspectorWindowName[192]{};

public:
    void SetWindowNames() noexcept {
        FormatImGuiWindowName("editor_toolbar", "", toolbarWindowName, sizeof(toolbarWindowName));
        FormatImGuiWindowName("hierarchy_shell", "Hierarchy", hierarchyWindowName, sizeof(hierarchyWindowName));
        FormatImGuiWindowName("project_shell", "Assets", projectWindowName, sizeof(projectWindowName));
        FormatImGuiWindowName("inspector_shell", "Inspector", inspectorWindowName, sizeof(inspectorWindowName));
    }
};

struct DockShellBinding {
    EditorDockShell* shell = nullptr;
};

void ToggleLeftPanelCallback(void* userData) {
    if (userData == nullptr) {
        return;
    }
    static_cast<DockShellBinding*>(userData)->shell->ToggleLeftPanel();
}

void ToggleRightPanelCallback(void* userData) {
    if (userData == nullptr) {
        return;
    }
    static_cast<DockShellBinding*>(userData)->shell->ToggleRightPanel();
}

struct GizmoToolbarBindingPtr {
    EditorGizmoToolbarBinding* binding = nullptr;
};

void SetGizmoTranslateCallback(void* userData) {
    if (userData == nullptr) {
        return;
    }
    auto* binding = static_cast<GizmoToolbarBindingPtr*>(userData)->binding;
    if (binding == nullptr || binding->viewport == nullptr) {
        return;
    }
    binding->viewport->SetGizmoMode(TransformGizmoMode::Translate);
    if (binding->statusLine != nullptr) {
        *binding->statusLine = Utf8String("Gizmo: Move (W/E/R).");
    }
}

void SetGizmoRotateCallback(void* userData) {
    if (userData == nullptr) {
        return;
    }
    auto* binding = static_cast<GizmoToolbarBindingPtr*>(userData)->binding;
    if (binding == nullptr || binding->viewport == nullptr) {
        return;
    }
    binding->viewport->SetGizmoMode(TransformGizmoMode::Rotate);
    if (binding->statusLine != nullptr) {
        *binding->statusLine = Utf8String("Gizmo: Rotate (W/E/R).");
    }
}

void SetGizmoScaleCallback(void* userData) {
    if (userData == nullptr) {
        return;
    }
    auto* binding = static_cast<GizmoToolbarBindingPtr*>(userData)->binding;
    if (binding == nullptr || binding->viewport == nullptr) {
        return;
    }
    binding->viewport->SetGizmoMode(TransformGizmoMode::Scale);
    if (binding->statusLine != nullptr) {
        *binding->statusLine = Utf8String("Gizmo: Scale (W/E/R).");
    }
}

}  // namespace

void EditorDockShell::SetGizmoBinding(EditorViewport* const viewportIn, Utf8String* const statusLineIn) noexcept {
    gizmoViewport = viewportIn;
    gizmoStatusLine = statusLineIn;
    gizmoBinding.viewport = viewportIn;
    gizmoBinding.statusLine = statusLineIn;
}

void EditorDockShell::SyncGizmoToolbar() noexcept {
    if (gizmoViewport == nullptr) {
        return;
    }
    const TransformGizmoMode mode = gizmoViewport->GetGizmoMode();
    if (gizmoMoveButton != nullptr) {
        gizmoMoveButton->SetLabel(mode == TransformGizmoMode::Translate ? Utf8String("✥ Move") : Utf8String("Move"));
    }
    if (gizmoRotateButton != nullptr) {
        gizmoRotateButton->SetLabel(mode == TransformGizmoMode::Rotate ? Utf8String("⟳ Rotate") : Utf8String("Rotate"));
    }
    if (gizmoScaleButton != nullptr) {
        gizmoScaleButton->SetLabel(mode == TransformGizmoMode::Scale ? Utf8String("⤢ Scale") : Utf8String("Scale"));
    }
}

void EditorDockShell::SetSidebarWidth(const float widthPx) noexcept {
    sidebarWidthPx = widthPx;
    if (auto* host = static_cast<EditorImGuiDockHost*>(dockHost)) {
        host->Configure(EditorDefaultLayout::kToolbarHeightPx, sidebarWidthPx, rightPanelWidthPx, leftStackSplit);
    }
}

float EditorDockShell::GetRightPanelWidth() const noexcept {
    return rightPanelWidthPx;
}

void EditorDockShell::ToggleLeftPanel() noexcept {
    if (auto* host = static_cast<EditorImGuiDockHost*>(dockHost)) {
        host->InvalidateLayout();
    }
}

void EditorDockShell::ToggleRightPanel() noexcept {
    if (auto* host = static_cast<EditorImGuiDockHost*>(dockHost)) {
        host->InvalidateLayout();
    }
}

void EditorDockShell::ApplyLayout(const Ui::SceneEditorLayoutSettings& layout) noexcept {
    sidebarWidthPx = layout.leftDockWidthPx > 0.0F ? layout.leftDockWidthPx : layout.sidebarWidthPx;
    rightPanelWidthPx = layout.rightDockWidthPx > 0.0F ? layout.rightDockWidthPx : EditorDefaultLayout::kRightWidthPx;
    leftStackSplit = layout.leftStackSplit > 0.0F ? layout.leftStackSplit : EditorDefaultLayout::kLeftStackSplit;
    Ui::SetSceneEditorSidebarWidthPx(sidebarWidthPx);
    Ui::SetSceneEditorSidebarSplit(leftStackSplit);
    if (auto* host = static_cast<EditorImGuiDockHost*>(dockHost)) {
        host->Configure(EditorDefaultLayout::kToolbarHeightPx, sidebarWidthPx, rightPanelWidthPx, leftStackSplit);
    }
}

void EditorDockShell::SyncLayout(const Ui::Rect& viewport) noexcept {
    if (root == nullptr) {
        return;
    }
    Ui::UiMeasureConstraints constraints{};
    constraints.maxWidth = viewport.width;
    constraints.maxHeight = viewport.height;
    Ui::UiSize desired{};
    root->Measure(constraints, desired);
    (void)desired;
    root->Arrange(viewport);
    if (auto* host = static_cast<EditorImGuiDockHost*>(dockHost)) {
        host->RefreshWorldViewportBounds();
    }
}

void EditorDockShell::SetPanels(
        UniquePtr<Ui::IUiElement> hierarchyRoot,
        UniquePtr<Ui::IUiElement> projectRoot,
        UniquePtr<Ui::IUiElement> inspectorRoot) {
    Rebuild(MoveTemp(hierarchyRoot), MoveTemp(projectRoot), MoveTemp(inspectorRoot));
}

void EditorDockShell::Rebuild(
        UniquePtr<Ui::IUiElement> hierarchyRoot,
        UniquePtr<Ui::IUiElement> projectRoot,
        UniquePtr<Ui::IUiElement> inspectorRoot) {
    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    auto dockHostUp = MakeUnique<EditorImGuiDockHost>();
    dockHost = dockHostUp.Get();
    static_cast<EditorImGuiDockHost*>(dockHost)->Configure(
            EditorDefaultLayout::kToolbarHeightPx, sidebarWidthPx, rightPanelWidthPx, leftStackSplit);
    static_cast<EditorImGuiDockHost*>(dockHost)->SetWindowNames();

    Ui::PanelDesc toolbarDesc{};
    toolbarDesc.id = Utf8String("editor_toolbar");
    toolbarDesc.title = Utf8String("");
    toolbarDesc.horizontalLayout = true;
    auto toolbar = factory.CreatePanel(toolbarDesc);

    Ui::LabelDesc titleDesc{};
    titleDesc.id = Utf8String("editor_title");
    titleDesc.text = Utf8String("Spark Editor");
    AdoptUiChild(*toolbar, factory.CreateLabel(titleDesc));

    static GizmoToolbarBindingPtr gizmoBindingPtr{};
    gizmoBindingPtr.binding = &gizmoBinding;

    Ui::LabelDesc gizmoSepDesc{};
    gizmoSepDesc.id = Utf8String("gizmo_sep");
    gizmoSepDesc.text = Utf8String("|");
    gizmoSepDesc.muted = true;
    AdoptUiChild(*toolbar, factory.CreateLabel(gizmoSepDesc));

    Ui::ButtonDesc moveDesc{};
    moveDesc.id = Utf8String("gizmo_move");
    moveDesc.label = Utf8String("✥ Move");
    auto moveUp = factory.CreateButton(moveDesc);
    gizmoMoveButton = moveUp.Get();
    Ui::UiVoidCallback moveCb{};
    moveCb.fn = &SetGizmoTranslateCallback;
    moveCb.userData = &gizmoBindingPtr;
    gizmoMoveButton->SetOnClick(moveCb);
    AdoptUiChild(*toolbar, MoveTemp(moveUp));

    Ui::ButtonDesc rotateDesc{};
    rotateDesc.id = Utf8String("gizmo_rotate");
    rotateDesc.label = Utf8String("Rotate");
    auto rotateUp = factory.CreateButton(rotateDesc);
    gizmoRotateButton = rotateUp.Get();
    Ui::UiVoidCallback rotateCb{};
    rotateCb.fn = &SetGizmoRotateCallback;
    rotateCb.userData = &gizmoBindingPtr;
    gizmoRotateButton->SetOnClick(rotateCb);
    AdoptUiChild(*toolbar, MoveTemp(rotateUp));

    Ui::ButtonDesc scaleDesc{};
    scaleDesc.id = Utf8String("gizmo_scale");
    scaleDesc.label = Utf8String("Scale");
    auto scaleUp = factory.CreateButton(scaleDesc);
    gizmoScaleButton = scaleUp.Get();
    Ui::UiVoidCallback scaleCb{};
    scaleCb.fn = &SetGizmoScaleCallback;
    scaleCb.userData = &gizmoBindingPtr;
    gizmoScaleButton->SetOnClick(scaleCb);
    AdoptUiChild(*toolbar, MoveTemp(scaleUp));

    static DockShellBinding toggleBinding{};
    toggleBinding.shell = this;

    Ui::ButtonDesc leftToggleDesc{};
    leftToggleDesc.id = Utf8String("toggle_left");
    leftToggleDesc.label = Utf8String("◀ Left");
    auto leftToggle = factory.CreateButton(leftToggleDesc);
    Ui::UiVoidCallback leftCb{};
    leftCb.fn = &ToggleLeftPanelCallback;
    leftCb.userData = &toggleBinding;
    leftToggle->SetOnClick(leftCb);
    AdoptUiChild(*toolbar, MoveTemp(leftToggle));

    Ui::ButtonDesc rightToggleDesc{};
    rightToggleDesc.id = Utf8String("toggle_right");
    rightToggleDesc.label = Utf8String("Right ▶");
    auto rightToggle = factory.CreateButton(rightToggleDesc);
    Ui::UiVoidCallback rightCb{};
    rightCb.fn = &ToggleRightPanelCallback;
    rightCb.userData = &toggleBinding;
    rightToggle->SetOnClick(rightCb);
    AdoptUiChild(*toolbar, MoveTemp(rightToggle));

    dockHostUp->AddChild(UniquePtr<Ui::IUiElement>(static_cast<Ui::IUiElement*>(toolbar.Release())));
    if (hierarchyRoot) {
        dockHostUp->AddChild(MoveTemp(hierarchyRoot));
    }
    if (projectRoot) {
        dockHostUp->AddChild(MoveTemp(projectRoot));
    }
    if (inspectorRoot) {
        dockHostUp->AddChild(MoveTemp(inspectorRoot));
    }

    root.Reset(dockHostUp.Release());
}

Ui::Rect EditorDockShell::GetWorldViewportRect() const noexcept {
    if (dockHost == nullptr) {
        return {};
    }
    return static_cast<const EditorImGuiDockHost*>(dockHost)->GetCenterBounds();
}

}  // namespace Spark::Editor
