#pragma once

#include "spark/editor/IEditorPanel.hpp"
#include "spark/editor/EditorTypes.hpp"
#include "spark/editor/inspector/InspectorEditTracker.hpp"
#include "spark/editor/inspector/InspectorWidgetRegistry.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/ui/controls/IUiControls.hpp"

namespace Spark {
class GameObject;
class IEngineContext;
}

namespace Spark::Editor {

class EditorCommandStack;
class EditorSelection;
class EditorViewport;

class InspectorPanel final : public IEditorPanel {
public:
    InspectorPanel();
    ~InspectorPanel() override = default;

    [[nodiscard]] Utf8String GetPanelId() const override { return Utf8String("inspector"); }
    [[nodiscard]] Utf8String GetDisplayName() const override { return Utf8String("Inspector"); }
    [[nodiscard]] Ui::IUiElement* GetRootElement() noexcept override { return root.Get(); }
    [[nodiscard]] UniquePtr<Ui::IUiElement> ReleaseRootElement() override { return MoveTemp(root); }

    void OnAttach(EditorContext& ctx) override;
    void OnTick(const FrameTiming& timing, EditorContext& ctx) override;
    void OnObjectNameCommitted();
    /** Sync widget callback context before UI input; call at the start of each frame. */
    void PrepareForFrame(EditorContext& ctx);
    void OnPostPaint(EditorContext& ctx);
    void CommitAllPendingEdits(EditorContext& ctx);

private:
    void EnsureBuilt();
    void UpdateWidgets(const InspectorWidgetContext& widgetCtx, bool commitPendingEdits, bool syncFromTarget);
    [[nodiscard]] InspectorWidgetContext BuildWidgetContext(const EditorContext& ctx) const noexcept;

    UniquePtr<Ui::IUiElement> root;
    Ui::IScrollPanel* scrollPanel = nullptr;
    Ui::ILabel* emptyLabel = nullptr;
    Ui::ITextBox* objectNameField = nullptr;
    InspectorWidgetRegistry widgetRegistry{};
    InspectorEditTracker<Utf8String> nameEditTracker{};
    Utf8String nameEditBefore{};
    EditorSelection* selection = nullptr;
    IEngineContext* engine = nullptr;
    EditorCommandStack* commandStack = nullptr;
    EditorViewport* viewport = nullptr;
    EditorMode mode = EditorMode::Edit;
    GameObject* lastPreparedTarget = nullptr;
    bool built = false;
};

}  // namespace Spark::Editor
