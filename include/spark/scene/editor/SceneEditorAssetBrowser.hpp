#pragma once

#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/scene/editor/SceneEditorAssetCatalog.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/IUiElement.hpp"
#include "spark/ui/factory/IUiControlsFactory.hpp"

namespace Spark {

class GameWorld;
class GameObject;

/**
 * Observer for asset-browser actions (Observer pattern).
 * The demo implements this to place prefabs, load scenes, and import glTF.
 */
class ISceneEditorAssetBrowserHost {
public:
    virtual ~ISceneEditorAssetBrowserHost() = default;

    virtual void OnAssetBrowserPlacePrefab(const SceneEditorAssetEntry& entry) = 0;
    virtual void OnAssetBrowserLoadScene(const SceneEditorAssetEntry& entry) = 0;
    virtual void OnAssetBrowserImportGltf() = 0;
    virtual void OnAssetBrowserRefreshCatalog() = 0;
};

/**
 * Left-sidebar retained UI: lists prefabs and scenes, with Place / Load / Import / Refresh.
 */
class SceneEditorAssetBrowser final {
public:
    void Mount(GameWorld& world, SceneEditorAssetCatalog& catalog, ISceneEditorAssetBrowserHost& host);
    void Unmount(GameWorld& world) noexcept;

    /** Builds list + action buttons into an existing panel (editor dock). */
    void BuildInto(
            Ui::IUiElement& parent,
            Ui::IUiControlsFactory& factory,
            SceneEditorAssetCatalog& catalog,
            ISceneEditorAssetBrowserHost& host);

    void RefreshListFromCatalog();
    void SetEnabled(bool enabled) noexcept;

    [[nodiscard]] UiCanvasComponent* GetCanvas() const noexcept { return canvas; }
    [[nodiscard]] bool IsMounted() const noexcept { return canvas != nullptr; }

private:
    struct Binding;

    static void OnListSelectionChanged(void* userData, int index) noexcept;
    static void OnListItemActivated(void* userData, int index) noexcept;
    static void OnPlaceClicked(void* userData) noexcept;
    static void OnLoadClicked(void* userData) noexcept;
    static void OnImportClicked(void* userData) noexcept;
    static void OnRefreshClicked(void* userData) noexcept;

    void RebuildListItems();
    void BuildControls(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory);
    [[nodiscard]] const SceneEditorAssetEntry* GetSelectedEntry() const noexcept;

    GameObject* uiRoot = nullptr;
    UiCanvasComponent* canvas = nullptr;
    SceneEditorAssetCatalog* catalog = nullptr;
    ISceneEditorAssetBrowserHost* host = nullptr;
    Ui::IList* list = nullptr;
    int selectedListIndex = -1;
};

}  // namespace Spark
