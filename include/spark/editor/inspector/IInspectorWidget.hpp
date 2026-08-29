#pragma once

#include "spark/editor/inspector/InspectorWidgetContext.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/factory/IUiControlsFactory.hpp"

namespace Spark {

class GameObject;

namespace Editor {

/**
 * Strategy for editing one component kind in the inspector.
 * Widgets build retained UI once, sync values from the target, and record undo on commit.
 */
class IInspectorWidget {
public:
    virtual ~IInspectorWidget() = default;

    [[nodiscard]] virtual ComponentKind GetComponentKind() const noexcept = 0;
    [[nodiscard]] virtual bool IsRelevantFor(const GameObject* target) const noexcept = 0;

    virtual void BuildUi(Ui::IUiElement& parent, Ui::IUiControlsFactory& factory) = 0;
    virtual void SetSectionVisible(bool visible) = 0;
    virtual void SyncFromTarget(const InspectorWidgetContext& ctx) = 0;
    virtual void CommitPendingEdits(const InspectorWidgetContext& ctx) = 0;
    virtual void CancelPendingEdits() = 0;
    /** Updates callback context before UI input is processed each frame. */
    virtual void PrepareFrameContext(const InspectorWidgetContext& ctx) = 0;
    [[nodiscard]] virtual bool HasPendingEdits() const noexcept { return false; }
};

}  // namespace Editor
}  // namespace Spark
