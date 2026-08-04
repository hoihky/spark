#include "spark/editor/panels/EditorDockShell.hpp"

#include "spark/ui/Ui.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/core/UiLayoutMetrics.hpp"
#include "spark/ui/spark/UiChild.hpp"

#include <algorithm>

namespace Spark::Editor {

namespace {

class EditorSideStack final : public Ui::UiElementBase {
public:
    EditorSideStack() : Ui::UiElementBase(Utf8String("editor_side_stack")) {}

    void SetTopFraction(const float fraction) noexcept { topFraction = std::clamp(fraction, 0.2F, 0.8F); }

protected:
    void DoMeasure(const Ui::UiMeasureConstraints& constraints, Ui::UiSize& outDesired) override {
        outDesired.width = constraints.maxWidth;
        outDesired.height = constraints.maxHeight;
    }

    void DoArrangeChildren() override {
        const Ui::Rect b = GetBounds();
        if (children.GetSize() < 2U) {
            if (children.GetSize() == 1U && children[0] != nullptr) {
                children[0]->Arrange(b);
            }
            return;
        }
        const float splitY = b.y + b.height * topFraction;
        const float topH = std::max(0.0F, splitY - b.y);
        const float botH = std::max(0.0F, b.y + b.height - splitY);
        if (children[0] != nullptr) {
            children[0]->Arrange(Ui::Rect{b.x, b.y, b.width, topH});
        }
        if (children[1] != nullptr) {
            children[1]->Arrange(Ui::Rect{b.x, splitY, b.width, botH});
        }
    }

    void DoPaint(Ui::IUiRenderer& /*renderer*/) override {}

private:
    float topFraction = 0.58F;
};

class EditorTopChrome final : public Ui::UiElementBase {
public:
    EditorTopChrome() : Ui::UiElementBase(Utf8String("editor_top_chrome")) {}

    void SetToolbarHeight(const float height) noexcept { toolbarHeight = height; }

protected:
    void DoMeasure(const Ui::UiMeasureConstraints& constraints, Ui::UiSize& outDesired) override {
        outDesired.width = constraints.maxWidth;
        outDesired.height = constraints.maxHeight;
    }

    void DoArrangeChildren() override {
        const Ui::Rect b = GetBounds();
        const Ui::UiLayoutMetrics& metrics = Ui::GetActiveUiLayoutMetrics();
        const float toolbarH = metrics.Scaled(toolbarHeight);
        if (children.GetSize() >= 1U && children[0] != nullptr) {
            children[0]->Arrange(Ui::Rect{b.x, b.y, b.width, toolbarH});
        }
        if (children.GetSize() >= 2U && children[1] != nullptr) {
            children[1]->Arrange(Ui::Rect{b.x, b.y + toolbarH, b.width, std::max(0.0F, b.height - toolbarH)});
        }
    }

    void DoPaint(Ui::IUiRenderer& /*renderer*/) override {}

private:
    float toolbarHeight = 40.0F;
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

}  // namespace

void EditorDockShell::SetSidebarWidth(const float widthPx) noexcept {
    sidebarWidthPx = widthPx;
    if (dockWorkspace != nullptr) {
        dockWorkspace->SetLeftWidth(widthPx);
    }
}

float EditorDockShell::GetRightPanelWidth() const noexcept {
    return dockWorkspace != nullptr ? dockWorkspace->GetRightWidth() : 320.0F;
}

void EditorDockShell::ToggleLeftPanel() noexcept {
    if (dockWorkspace != nullptr) {
        dockWorkspace->ToggleLeftCollapsed();
    }
}

void EditorDockShell::ToggleRightPanel() noexcept {
    if (dockWorkspace != nullptr) {
        dockWorkspace->ToggleRightCollapsed();
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

    Ui::DockWorkspaceDesc dockDesc{};
    dockDesc.id = Utf8String("editor_dock");
    dockDesc.leftWidth = sidebarWidthPx;
    dockDesc.rightWidth = 320.0F;
    auto dockUp = factory.CreateDockWorkspace(dockDesc);
    dockWorkspace = dockUp.Get();
    dockWorkspace->SetLeftWidth(sidebarWidthPx);

    if (Ui::IUiElement* leftPane = dockWorkspace->GetLeftPane()) {
        auto sideStack = MakeUnique<EditorSideStack>();
        if (hierarchyRoot) {
            sideStack->AddChild(UniquePtr<Ui::IUiElement>(hierarchyRoot.Release()));
        }
        if (projectRoot) {
            sideStack->AddChild(UniquePtr<Ui::IUiElement>(projectRoot.Release()));
        }
        AdoptUiChild(*leftPane, MoveTemp(sideStack));
    }
    if (Ui::IUiElement* rightPane = dockWorkspace->GetRightPane(); rightPane != nullptr && inspectorRoot) {
        AdoptUiChild(*rightPane, MoveTemp(inspectorRoot));
    }

    auto chrome = MakeUnique<EditorTopChrome>();

    Ui::PanelDesc toolbarDesc{};
    toolbarDesc.id = Utf8String("editor_toolbar");
    toolbarDesc.title = Utf8String("");
    auto toolbar = factory.CreatePanel(toolbarDesc);

    Ui::LabelDesc titleDesc{};
    titleDesc.id = Utf8String("editor_title");
    titleDesc.text = Utf8String("Spark Editor");
    AdoptUiChild(*toolbar, factory.CreateLabel(titleDesc));

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

    chrome->AddChild(UniquePtr<Ui::IUiElement>(static_cast<Ui::IUiElement*>(toolbar.Release())));
    chrome->AddChild(UniquePtr<Ui::IUiElement>(static_cast<Ui::IUiElement*>(dockUp.Release())));
    root.Reset(chrome.Release());
}

Ui::Rect EditorDockShell::GetWorldViewportRect() const noexcept {
    if (dockWorkspace == nullptr) {
        return {};
    }
    return dockWorkspace->GetCenterBounds();
}

}  // namespace Spark::Editor
