#include "spark/scene/editor/SceneEditorAssetBrowser.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/ui/runtime/EditorLayoutStore.hpp"
#include "spark/ui/runtime/IUiBackend.hpp"
#include "spark/ui/spark/UiChild.hpp"
#include "spark/ui/runtime/UiSystem.hpp"

namespace Spark {

struct SceneEditorAssetBrowser::Binding {
    SceneEditorAssetBrowser* browser = nullptr;
};

void SceneEditorAssetBrowser::Mount(
        GameWorld& world,
        SceneEditorAssetCatalog& catalog,
        ISceneEditorAssetBrowserHost& host) {
    if (canvas_ != nullptr) {
        return;
    }
    catalog_ = &catalog;
    host_ = &host;
    catalog.Refresh();

    uiRoot_ = world.CreateGameObject();
    uiRoot_->GetName() = Utf8String("SceneEditorAssetBrowser");
    canvas_ = uiRoot_->AddComponent<UiCanvasComponent>();
    canvas_->SetSortOrder(150);
    canvas_->SetTheme(Ui::UiTheme::ClassicMint());

    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    Ui::PanelDesc panelDesc{};
    panelDesc.id = Utf8String("asset_browser");
    panelDesc.title = Utf8String("Assets");
    panelDesc.width = Ui::GetSceneEditorSidebarWidthPx() - 16.0F;
    panelDesc.height = 0.0F;
    panelDesc.anchorRight = false;
    panelDesc.edgeMargin = 8.0F;
    auto panel = factory.CreatePanel(panelDesc);

    Ui::LabelDesc hintDesc{};
    hintDesc.id = Utf8String("asset_hint");
    hintDesc.text = Utf8String("Prefabs & scenes. Double-click or use buttons below.");
    hintDesc.muted = true;
    Ui::AdoptUiChild(*panel, factory.CreateLabel(hintDesc));

    Ui::ListDesc listDesc{};
    listDesc.id = Utf8String("asset_list");
    listDesc.rowHeight = 26.0F;
    listDesc.itemFontSize = 14.0F;
    listDesc.verticalScrollingEnabled = true;
    listDesc.fillRemainingHeight = true;
    auto list = factory.CreateList(listDesc);
    list_ = list.Get();
  static Binding listBinding{};
    listBinding.browser = this;
    Ui::UiIntCallback selectCb{};
    selectCb.fn = &SceneEditorAssetBrowser::OnListSelectionChanged;
    selectCb.userData = &listBinding;
    list->SetOnSelectionChanged(selectCb);
    Ui::AdoptUiChild(*panel, MoveTemp(list));

    Ui::SeparatorDesc sepDesc{};
    sepDesc.id = Utf8String("asset_sep");
    Ui::AdoptUiChild(*panel, factory.CreateSeparator(sepDesc));

    static Binding placeBinding{};
    placeBinding.browser = this;
    Ui::ButtonDesc placeDesc{};
    placeDesc.id = Utf8String("place_prefab");
    placeDesc.label = Utf8String("Place prefab");
    auto placeBtn = factory.CreateButton(placeDesc);
    Ui::UiVoidCallback placeCb{};
    placeCb.fn = &SceneEditorAssetBrowser::OnPlaceClicked;
    placeCb.userData = &placeBinding;
    placeBtn->SetOnClick(placeCb);
    Ui::AdoptUiChild(*panel, MoveTemp(placeBtn));

    Ui::ButtonDesc loadDesc{};
    loadDesc.id = Utf8String("load_scene");
    loadDesc.label = Utf8String("Load scene");
    auto loadBtn = factory.CreateButton(loadDesc);
    static Binding loadBinding{};
    loadBinding.browser = this;
    Ui::UiVoidCallback loadCb{};
    loadCb.fn = &SceneEditorAssetBrowser::OnLoadClicked;
    loadCb.userData = &loadBinding;
    loadBtn->SetOnClick(loadCb);
    Ui::AdoptUiChild(*panel, MoveTemp(loadBtn));

    Ui::ButtonDesc importDesc{};
    importDesc.id = Utf8String("import_gltf");
    importDesc.label = Utf8String("Import GLTF…");
    auto importBtn = factory.CreateButton(importDesc);
    static Binding importBinding{};
    importBinding.browser = this;
    Ui::UiVoidCallback importCb{};
    importCb.fn = &SceneEditorAssetBrowser::OnImportClicked;
    importCb.userData = &importBinding;
    importBtn->SetOnClick(importCb);
    Ui::AdoptUiChild(*panel, MoveTemp(importBtn));

    Ui::ButtonDesc refreshDesc{};
    refreshDesc.id = Utf8String("refresh_assets");
    refreshDesc.label = Utf8String("Refresh list");
    auto refreshBtn = factory.CreateButton(refreshDesc);
    static Binding refreshBinding{};
    refreshBinding.browser = this;
    Ui::UiVoidCallback refreshCb{};
    refreshCb.fn = &SceneEditorAssetBrowser::OnRefreshClicked;
    refreshCb.userData = &refreshBinding;
    refreshBtn->SetOnClick(refreshCb);
    Ui::AdoptUiChild(*panel, MoveTemp(refreshBtn));

    canvas_->SetRoot(MoveTemp(panel));
    RebuildListItems();
}

void SceneEditorAssetBrowser::Unmount(GameWorld& world) noexcept {
    if (uiRoot_ != nullptr) {
        world.DestroyGameObject(uiRoot_);
        uiRoot_ = nullptr;
    }
    canvas_ = nullptr;
    list_ = nullptr;
    catalog_ = nullptr;
    host_ = nullptr;
    selectedListIndex_ = -1;
}

void SceneEditorAssetBrowser::SetEnabled(const bool enabled) noexcept {
    if (canvas_ != nullptr) {
        canvas_->SetCanvasEnabled(enabled);
    }
}

void SceneEditorAssetBrowser::RefreshListFromCatalog() {
    RebuildListItems();
}

void SceneEditorAssetBrowser::RebuildListItems() {
    if (list_ == nullptr || catalog_ == nullptr) {
        return;
    }
    Array<Utf8String> labels;
    const Array<SceneEditorAssetEntry>& entries = catalog_->GetEntries();
    labels.Reserve(entries.GetSize());
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        labels.PushBack(entries[i].displayName);
    }
    list_->SetItems(MoveTemp(labels));
    if (!labels.IsEmpty() && selectedListIndex_ < 0) {
        list_->SetSelectedIndex(0);
        selectedListIndex_ = 0;
    }
}

const SceneEditorAssetEntry* SceneEditorAssetBrowser::GetSelectedEntry() const noexcept {
    if (catalog_ == nullptr || selectedListIndex_ < 0) {
        return nullptr;
    }
    return catalog_->FindByListIndex(selectedListIndex_);
}

void SceneEditorAssetBrowser::OnListSelectionChanged(void* userData, const int index) noexcept {
    auto* binding = static_cast<Binding*>(userData);
    if (binding == nullptr || binding->browser == nullptr) {
        return;
    }
    binding->browser->selectedListIndex_ = index;
}

void SceneEditorAssetBrowser::OnPlaceClicked(void* userData) noexcept {
    auto* binding = static_cast<Binding*>(userData);
    if (binding == nullptr || binding->browser == nullptr || binding->browser->host_ == nullptr) {
        return;
    }
    const SceneEditorAssetEntry* entry = binding->browser->GetSelectedEntry();
    if (entry == nullptr) {
        return;
    }
    if (entry->kind != SceneEditorAssetKind::Prefab) {
        return;
    }
    binding->browser->host_->OnAssetBrowserPlacePrefab(*entry);
}

void SceneEditorAssetBrowser::OnLoadClicked(void* userData) noexcept {
    auto* binding = static_cast<Binding*>(userData);
    if (binding == nullptr || binding->browser == nullptr || binding->browser->host_ == nullptr) {
        return;
    }
    const SceneEditorAssetEntry* entry = binding->browser->GetSelectedEntry();
    if (entry == nullptr) {
        return;
    }
    if (entry->kind != SceneEditorAssetKind::Scene) {
        return;
    }
    binding->browser->host_->OnAssetBrowserLoadScene(*entry);
}

void SceneEditorAssetBrowser::OnImportClicked(void* userData) noexcept {
    auto* binding = static_cast<Binding*>(userData);
    if (binding == nullptr || binding->browser == nullptr || binding->browser->host_ == nullptr) {
        return;
    }
    binding->browser->host_->OnAssetBrowserImportGltf();
}

void SceneEditorAssetBrowser::OnRefreshClicked(void* userData) noexcept {
    auto* binding = static_cast<Binding*>(userData);
    if (binding == nullptr || binding->browser == nullptr || binding->browser->host_ == nullptr) {
        return;
    }
    binding->browser->host_->OnAssetBrowserRefreshCatalog();
}

}  // namespace Spark
