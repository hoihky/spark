#include "spark/editor/panels/EditorDockShell.hpp"

#include "spark/gui/EditorLayoutStore.hpp"
#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiTheme.hpp"
#include "spark/gui/controls/Label.hpp"
#include "spark/gui/controls/MenuBar.hpp"
#include "spark/gui/controls/Panel.hpp"
#include "spark/gui/docking/DockFrameLayout.hpp"
#include "spark/gui/docking/DockManager.hpp"
#include "spark/gui/docking/DockPanel.hpp"
#include "spark/gui/docking/DockSidePane.hpp"

namespace Spark::Editor {

namespace {

class EditorTopChromeLayout final : public Gui::Widget {
public:
    void SetMenuHeight(float h) noexcept { menuHeight_ = h; }
    void SetToolbarHeight(float h) noexcept { toolbarHeight_ = h; }

    void Arrange(const Gui::Rect& r) override {
        bounds = r;
        const auto& ch = GetChildren();
        if (ch.IsEmpty()) {
            return;
        }
        const Gui::GuiLayoutMetrics& m = Gui::GetActiveGuiLayoutMetrics();
        const float menuH = m.Scaled(menuHeight_);
        const float toolbarH = m.Scaled(toolbarHeight_);
        float y = r.y;
        if (ch.GetSize() >= 1U && ch[0]) {
            ch[0]->Arrange({r.x, y, r.width, menuH});
            y += menuH;
        }
        if (ch.GetSize() >= 2U && ch[1]) {
            ch[1]->Arrange({r.x, y, r.width, toolbarH});
            y += toolbarH;
        }
        if (ch.GetSize() >= 3U && ch[2]) {
            ch[2]->Arrange({r.x, y, r.width, std::max(0.0F, r.y + r.height - y)});
        }
    }

    void Paint(Gui::GuiPaintContext& ctx) const override { PaintChildren(ctx); }

    [[nodiscard]] Gui::Widget* FindDeepestHover(const float x, const float y) override {
        const auto& ch = GetChildren();
        for (std::size_t i = ch.GetSize(); i > 0U; --i) {
            if (ch[i - 1U]) {
                if (Gui::Widget* hit = ch[i - 1U]->FindDeepestHover(x, y)) {
                    return hit;
                }
            }
        }
        return nullptr;
    }

private:
    float menuHeight_ = 34.0F;
    float toolbarHeight_ = 36.0F;
};

void StyleChromePanel(Gui::Panel& panel) {
    const Gui::GuiTheme& th = Gui::GuiTheme::SceneEditorDark();
    panel.SetBackgroundGradient(th.panelElevatedTop, th.panelElevatedBottom, th.panelElevatedAlpha);
    panel.SetChromeEnabled(true);
    panel.SetDropShadowEnabled(false);
}

UniquePtr<Gui::Widget> WrapPanelChrome(UniquePtr<Gui::Widget> inner) {
    auto shell = MakeUnique<Gui::Panel>();
    StyleChromePanel(*shell);
    shell->SetPadding(4.0F);
    if (inner) {
        shell->AddChild(MoveTemp(inner));
    }
    return UniquePtr<Gui::Widget>(shell.Release());
}

}  // namespace

void EditorDockShell::SetSidebarWidth(const float widthPx) noexcept {
    sidebarWidthPx_ = widthPx;
}

void EditorDockShell::ToggleLeftPanel() noexcept {
    if (dockFrame_ != nullptr && dockFrame_->GetLeftPane() != nullptr) {
        dockFrame_->GetLeftPane()->ToggleCollapsed();
    } else {
        dockManager_.ToggleLeftCollapsed();
    }
}

void EditorDockShell::ToggleRightPanel() noexcept {
    if (dockFrame_ != nullptr && dockFrame_->GetRightPane() != nullptr) {
        dockFrame_->GetRightPane()->ToggleCollapsed();
    } else {
        dockManager_.ToggleRightCollapsed();
    }
}

void EditorDockShell::SetPanels(
        UniquePtr<Gui::Widget> hierarchyRoot,
        UniquePtr<Gui::Widget> projectRoot,
        UniquePtr<Gui::Widget> inspectorRoot) {
    Rebuild(MoveTemp(hierarchyRoot), MoveTemp(projectRoot), MoveTemp(inspectorRoot));
}

void EditorDockShell::Rebuild(
        UniquePtr<Gui::Widget> hierarchyRoot,
        UniquePtr<Gui::Widget> projectRoot,
        UniquePtr<Gui::Widget> inspectorRoot) {
    dockManager_ = Gui::DockManager::CreateEditorDefault();
    Gui::DockLayoutState state = dockManager_.GetLayoutState();
    state.leftWidthPx = sidebarWidthPx_;
    dockManager_.SetLayoutState(state);

    if (hierarchyRoot) {
        dockManager_.RegisterPanel(MakeUnique<Gui::DockPanel>(
                Utf8String("hierarchy"),
                Utf8String("Scene"),
                WrapPanelChrome(MoveTemp(hierarchyRoot)),
                Gui::DockSide::Left));
    }
    if (projectRoot) {
        dockManager_.RegisterPanel(MakeUnique<Gui::DockPanel>(
                Utf8String("project"),
                Utf8String("Project"),
                WrapPanelChrome(MoveTemp(projectRoot)),
                Gui::DockSide::Left));
    }
    if (inspectorRoot) {
        dockManager_.RegisterPanel(MakeUnique<Gui::DockPanel>(
                Utf8String("inspector"),
                Utf8String("Inspector"),
                WrapPanelChrome(MoveTemp(inspectorRoot)),
                Gui::DockSide::Right));
    }

    dockManager_.SetOnLayoutStateChanged([](const Gui::DockLayoutState& layoutState) {
        Gui::SceneEditorLayoutSettings editorLayout{};
        editorLayout.sidebarWidthPx = layoutState.leftWidthPx;
        (void)Gui::SaveSceneEditorLayout(editorLayout);
    });

    auto dockFrame = dockManager_.BuildFrame();
    dockFrame_ = dockFrame.Get();

    auto outer = MakeUnique<EditorTopChromeLayout>();

    Array<Gui::MenuBarItem> menus;
    {
        Gui::MenuBarItem file{};
        file.label = Utf8String("File");
        file.dropdownEntries.PushBack(Utf8String("New Project"));
        file.dropdownEntries.PushBack(Utf8String("Open Project"));
        file.dropdownEntries.PushBack(Utf8String("Save Project"));
        menus.PushBack(MoveTemp(file));
    }
    {
        Gui::MenuBarItem scene{};
        scene.label = Utf8String("Scene");
        scene.dropdownEntries.PushBack(Utf8String("New Scene"));
        scene.dropdownEntries.PushBack(Utf8String("Save Scene"));
        scene.dropdownEntries.PushBack(Utf8String("Load Scene"));
        menus.PushBack(MoveTemp(scene));
    }
    {
        Gui::MenuBarItem view{};
        view.label = Utf8String("View");
        view.dropdownEntries.PushBack(Utf8String("Toggle Left Panel"));
        view.dropdownEntries.PushBack(Utf8String("Toggle Right Panel"));
        view.dropdownEntries.PushBack(Utf8String("Focus Selection"));
        view.dropdownEntries.PushBack(Utf8String("Reset Camera"));
        view.onDropdownSelect = [this](const int row) {
            if (row == 0) {
                ToggleLeftPanel();
            } else if (row == 1) {
                ToggleRightPanel();
            }
        };
        menus.PushBack(MoveTemp(view));
    }
    auto menuBar = MakeUnique<Gui::MenuBar>();
    menuBar->SetItems(MoveTemp(menus));
    menuBar->SetBarHeight(34.0F);
    outer->AddChild(MoveTemp(menuBar));

    auto toolbar = MakeUnique<Gui::Panel>();
    StyleChromePanel(*toolbar);
    toolbar->SetPadding(6.0F);
    toolbar->SetChromeEnabled(false);
    toolbar->SetHitTest(false);
    auto toolbarTitle = MakeUnique<Gui::Label>();
    toolbarTitle->SetText(Utf8String("Spark Editor"));
    toolbarTitle->SetFontSize(20.0F);
    toolbarTitle->SetTextColor(Gui::GuiTheme::SceneEditorDark().labelPrimary);
    toolbar->AddChild(MoveTemp(toolbarTitle));
    outer->AddChild(MoveTemp(toolbar));

    outer->AddChild(MoveTemp(dockFrame));
    root_.Reset(outer.Release());
}

Gui::Rect EditorDockShell::GetWorldViewportRect() const noexcept {
    if (dockFrame_ == nullptr) {
        return {};
    }
    return dockFrame_->GetCenterBounds();
}

}  // namespace Spark::Editor
