#pragma once

#include "spark/editor/IEditorPanel.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/editor/SceneEditorAssetBrowser.hpp"
#include "spark/scene/editor/SceneEditorAssetCatalog.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark::Editor {

class ProjectBrowserPanel final : public IEditorPanel {
public:
    ProjectBrowserPanel();
    ~ProjectBrowserPanel() override = default;

    [[nodiscard]] Utf8String GetPanelId() const override { return Utf8String("project"); }
    [[nodiscard]] Utf8String GetDisplayName() const override { return Utf8String("Assets"); }
    [[nodiscard]] Ui::IUiElement* GetRootElement() noexcept override { return root.Get(); }
    [[nodiscard]] UniquePtr<Ui::IUiElement> ReleaseRootElement() override { return MoveTemp(root); }

    void OnAttach(EditorContext& ctx) override;
    void OnTick(const FrameTiming& timing, EditorContext& ctx) override;

    void BindAssetCatalog(SceneEditorAssetCatalog& catalog) noexcept { assetCatalog = &catalog; }
    SceneEditorAssetBrowser& GetAssetBrowser() noexcept { return assetBrowser; }

private:
    void EnsureBuilt();

    UniquePtr<Ui::IUiElement> root;
    SceneEditorAssetCatalog* assetCatalog = nullptr;
    ISceneEditorAssetBrowserHost* assetHost = nullptr;
    SceneEditorAssetBrowser assetBrowser{};
    bool built = false;
};

}  // namespace Spark::Editor
