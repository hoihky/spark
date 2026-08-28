#pragma once

#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/scene/editor/SceneEditorAssetCatalog.hpp"
#include "spark/ui/controls/IUiControls.hpp"

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

    void RefreshListFromCatalog();
    void SetEnabled(bool enabled) noexcept;

    [[nodiscard]] UiCanvasComponent* GetCanvas() const noexcept { return canvas_; }
    [[nodiscard]] bool IsMounted() const noexcept { return canvas_ != nullptr; }

private:
    struct Binding;

    static void OnListSelectionChanged(void* userData, int index) noexcept;
    static void OnPlaceClicked(void* userData) noexcept;
    static void OnLoadClicked(void* userData) noexcept;
    static void OnImportClicked(void* userData) noexcept;
    static void OnRefreshClicked(void* userData) noexcept;

    void RebuildListItems();
    [[nodiscard]] const SceneEditorAssetEntry* GetSelectedEntry() const noexcept;

    GameObject* uiRoot_ = nullptr;
    UiCanvasComponent* canvas_ = nullptr;
    SceneEditorAssetCatalog* catalog_ = nullptr;
    ISceneEditorAssetBrowserHost* host_ = nullptr;
    Ui::IList* list_ = nullptr;
    int selectedListIndex_ = -1;
};

}  // namespace Spark
