#include "spark/gui/docking/DockManager.hpp"

#include "spark/gui/controls/Panel.hpp"
#include "spark/gui/controls/TabControl.hpp"
#include "spark/gui/docking/DockFrameLayout.hpp"
#include "spark/gui/docking/DockSidePane.hpp"

namespace Spark::Gui {

void DockManager::RegisterPanel(UniquePtr<DockPanel> panel) {
    if (panel) {
        panels_.PushBack(MoveTemp(panel));
    }
}

DockPanel* DockManager::FindPanel(const Utf8String& id) noexcept {
    for (std::size_t i = 0; i < panels_.GetSize(); ++i) {
        if (panels_[i] && panels_[i]->GetId() == id) {
            return panels_[i].Get();
        }
    }
    return nullptr;
}

UniquePtr<DockFrameLayout> DockManager::BuildFrame() {
    auto frame = MakeUnique<DockFrameLayout>();

    auto leftPane = MakeUnique<DockSidePane>();
    leftPane->SetEdge(DockSidePane::Edge::Left);
    leftPane->SetTitle(Utf8String("Panels"));
    leftPane->SetPaneWidth(state_.leftWidthPx);
    leftPane->SetCollapsed(state_.leftCollapsed);
    leftPane->SetContent(BuildLeftContent());
    leftPane->SetOnPaneWidthChanged([this](const float widthPx, const bool committed) {
        state_.leftWidthPx = widthPx;
        NotifyStateChanged(committed);
    });
    leftPane->SetOnCollapsedChanged([this](const bool collapsed) {
        state_.leftCollapsed = collapsed;
        NotifyStateChanged(true);
    });

    auto centerUp = MakeUnique<Panel>();
    centerUp->SetHitTest(false);
    centerUp->SetBackgroundEnabled(false);
    centerUp->SetChromeEnabled(false);
    centerUp->SetDropShadowEnabled(false);

    auto rightPane = MakeUnique<DockSidePane>();
    rightPane->SetEdge(DockSidePane::Edge::Right);
    rightPane->SetTitle(Utf8String("Inspector"));
    rightPane->SetPaneWidth(state_.rightWidthPx);
    rightPane->SetCollapsed(state_.rightCollapsed);
    rightPane->SetContent(BuildRightContent());
    rightPane->SetOnPaneWidthChanged([this](const float widthPx, const bool committed) {
        state_.rightWidthPx = widthPx;
        NotifyStateChanged(committed);
    });
    rightPane->SetOnCollapsedChanged([this](const bool collapsed) {
        state_.rightCollapsed = collapsed;
        NotifyStateChanged(true);
    });

    frame->SetLeftPane(MoveTemp(leftPane));
    frame->SetCenter(UniquePtr<Widget>(static_cast<Widget*>(centerUp.Release())));
    frame->SetRightPane(MoveTemp(rightPane));
    return frame;
}

void DockManager::LoadState() noexcept {
    DockLayoutState loaded{};
    if (TryLoadDockLayoutState(loaded)) {
        state_ = loaded;
    }
}

void DockManager::SaveState() const noexcept {
    (void)SaveDockLayoutState(state_);
}

DockManager DockManager::CreateEditorDefault() {
    DockManager manager;
    manager.LoadState();
    return manager;
}

void DockManager::NotifyStateChanged(const bool save) {
    if (onLayoutStateChanged_) {
        onLayoutStateChanged_(state_);
    }
    if (save) {
        SaveState();
    }
}

UniquePtr<Widget> DockManager::BuildLeftContent() {
    auto tabs = MakeUnique<TabControl>();
    tabs->SetTabBarHeight(32.0F);
    bool added = false;
    for (std::size_t i = 0; i < panels_.GetSize(); ++i) {
        DockPanel* panel = panels_[i].Get();
        if (panel == nullptr || panel->GetSide() != DockSide::Left) {
            continue;
        }
        UniquePtr<Widget> content = panel->ReleaseContent();
        if (content) {
            tabs->AddTabWidget(panel->GetTitle(), MoveTemp(content));
            added = true;
        }
    }
    if (!added) {
        auto empty = MakeUnique<Panel>();
        empty->SetBackgroundEnabled(false);
        empty->SetChromeEnabled(false);
        return UniquePtr<Widget>(empty.Release());
    }
    tabs->SetSelectedIndex(state_.leftSelectedTab);
    tabs->SetOnTabChanged([this](const int tabIndex) {
        state_.leftSelectedTab = tabIndex;
        NotifyStateChanged(true);
    });
    return UniquePtr<Widget>(tabs.Release());
}

UniquePtr<Widget> DockManager::BuildRightContent() {
    for (std::size_t i = 0; i < panels_.GetSize(); ++i) {
        DockPanel* panel = panels_[i].Get();
        if (panel == nullptr || panel->GetSide() != DockSide::Right) {
            continue;
        }
        return panel->ReleaseContent();
    }
    auto empty = MakeUnique<Panel>();
    empty->SetBackgroundEnabled(false);
    empty->SetChromeEnabled(false);
    return UniquePtr<Widget>(empty.Release());
}

}  // namespace Spark::Gui
