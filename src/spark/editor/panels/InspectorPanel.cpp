#include "spark/editor/panels/InspectorPanel.hpp"

#include "spark/editor/EditorSelection.hpp"
#include "spark/editor/EditorViewport.hpp"
#include "spark/editor/commands/SetGameObjectNameCommand.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/editor/inspector/InspectorUiBuilder.hpp"
#include "spark/ui/Ui.hpp"
#include "spark/ui/spark/UiChild.hpp"

#include <GLFW/glfw3.h>

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#endif

namespace Spark::Editor {

namespace {

void OnObjectNameCommittedStatic(void* userData) {
    static_cast<InspectorPanel*>(userData)->OnObjectNameCommitted();
}

}  // namespace

InspectorPanel::InspectorPanel() = default;

void InspectorPanel::EnsureBuilt() {
    if (built) {
        return;
    }
    Ui::IUiControlsFactory& factory = Ui::UiSystem::Get().GetActiveBackendPtr()->GetControlsFactory();

    Ui::PanelDesc shellDesc{};
    shellDesc.id = Utf8String("inspector_shell");
    shellDesc.title = Utf8String("Inspector");
    auto shell = factory.CreatePanel(shellDesc);

    Ui::ScrollPanelDesc scrollDesc{};
    scrollDesc.id = Utf8String("inspector_scroll");
    scrollDesc.height = 520.0F;
    auto scrollUp = factory.CreateScrollPanel(scrollDesc);
    scrollPanel = scrollUp.Get();
    AdoptUiChild(*shell, MoveTemp(scrollUp));

    Ui::LabelDesc emptyDesc{};
    emptyDesc.id = Utf8String("inspector_empty");
    emptyDesc.text = Utf8String("Select an entity in the Hierarchy.");
    emptyDesc.muted = true;
    auto emptyUp = factory.CreateLabel(emptyDesc);
    emptyLabel = emptyUp.Get();
    AdoptUiChild(*scrollPanel, MoveTemp(emptyUp));

    Ui::TextFieldDesc nameDesc{};
    nameDesc.id = Utf8String("inspector_object_name");
    nameDesc.label = Utf8String("Name");
    nameDesc.text = Utf8String("");
    auto nameUp = factory.CreateTextBox(nameDesc);
    objectNameField = nameUp.Get();
    Ui::UiVoidCallback commitCb{};
    commitCb.fn = OnObjectNameCommittedStatic;
    commitCb.userData = this;
    objectNameField->SetOnCommit(commitCb);
    InspectorUiBuilder::SetVisible(objectNameField, false);
    AdoptUiChild(*scrollPanel, MoveTemp(nameUp));

    const Array<UniquePtr<IInspectorWidget>>& widgets = widgetRegistry.GetWidgets();
    for (std::size_t i = 0; i < widgets.GetSize(); ++i) {
        const UniquePtr<IInspectorWidget>& widget = widgets[i];
        if (widget != nullptr) {
            widget->BuildUi(*scrollPanel, factory);
            widget->SetSectionVisible(false);
        }
    }

    root.Reset(static_cast<Ui::IUiElement*>(shell.Release()));
    built = true;
}

void InspectorPanel::OnAttach(EditorContext& ctx) {
    EnsureBuilt();
    selection = ctx.selection;
    engine = ctx.engine;
    commandStack = ctx.commandStack;
    viewport = ctx.viewport;
    mode = ctx.mode;
}

InspectorWidgetContext InspectorPanel::BuildWidgetContext(const EditorContext& ctx) const noexcept {
    InspectorWidgetContext widgetCtx{};
    widgetCtx.target = selection != nullptr ? selection->GetPrimary() : nullptr;
    widgetCtx.world = ctx.world;
    widgetCtx.commandStack = ctx.commandStack;
    widgetCtx.textureCatalog = ctx.textureCatalog;
    widgetCtx.mode = ctx.mode;
    if (viewport != nullptr) {
        widgetCtx.externalTransformEditActive = viewport->IsTransformEditActive();
        widgetCtx.externalTransformTarget = viewport->GetTransformEditTarget();
    }
    return widgetCtx;
}

void InspectorPanel::PrepareForFrame(EditorContext& ctx) {
    if (!built) {
        return;
    }
    mode = ctx.mode;
    commandStack = ctx.commandStack;
    engine = ctx.engine;
    viewport = ctx.viewport;

    const InspectorWidgetContext widgetCtx = BuildWidgetContext(ctx);
    GameObject* const target = widgetCtx.target;
    if (target != lastPreparedTarget) {
        const Array<UniquePtr<IInspectorWidget>>& widgets = widgetRegistry.GetWidgets();
        for (std::size_t i = 0; i < widgets.GetSize(); ++i) {
            if (widgets[i] != nullptr) {
                widgets[i]->CancelPendingEdits();
            }
        }
        nameEditTracker.Cancel();
        lastPreparedTarget = target;
        UpdateWidgets(widgetCtx, false, true);
    }
    widgetRegistry.PrepareFrameContext(widgetCtx);
}

void InspectorPanel::OnPostPaint(EditorContext& ctx) {
    if (!built || selection == nullptr) {
        return;
    }
    mode = ctx.mode;
    commandStack = ctx.commandStack;
    engine = ctx.engine;
    viewport = ctx.viewport;

    const InspectorWidgetContext widgetCtx = BuildWidgetContext(ctx);

#if SPARK_ENABLE_IMGUI
    const bool uiItemActive = ImGui::IsAnyItemActive();
#else
    const bool uiItemActive = false;
#endif
    const bool anyPendingEdits = widgetRegistry.HasAnyPendingEdits();
    const bool commitPendingEdits = anyPendingEdits && !uiItemActive;
    const bool syncFromTarget = !uiItemActive && !anyPendingEdits;

    UpdateWidgets(widgetCtx, commitPendingEdits, syncFromTarget);
}

void InspectorPanel::OnObjectNameCommitted() {
    if (selection == nullptr || commandStack == nullptr || objectNameField == nullptr || mode != EditorMode::Edit) {
        return;
    }
    GameObject* target = selection->GetPrimary();
    if (target == nullptr) {
        return;
    }
    const Utf8String after(objectNameField->GetText().CStr());
    if (after == nameEditBefore) {
        nameEditTracker.Cancel();
        return;
    }
    auto command = MakeUnique<SetGameObjectNameCommand>(*target, nameEditBefore, after);
    commandStack->Record(UniquePtr<IEditorCommand>(command.Release()));
    nameEditTracker.Cancel();
}

void InspectorPanel::UpdateWidgets(
        const InspectorWidgetContext& widgetCtx,
        const bool commitPendingEdits,
        const bool syncFromTarget) {
    GameObject* target = widgetCtx.target;
    const bool hasSelection = target != nullptr;

    if (emptyLabel != nullptr) {
        InspectorUiBuilder::SetVisible(emptyLabel, !hasSelection);
    }
    if (objectNameField != nullptr) {
        InspectorUiBuilder::SetVisible(objectNameField, hasSelection);
        if (hasSelection && !objectNameField->IsEditing()) {
            objectNameField->SetText(target->GetName());
        }
    }

    const Array<UniquePtr<IInspectorWidget>>& widgets = widgetRegistry.GetWidgets();
    for (std::size_t i = 0; i < widgets.GetSize(); ++i) {
        const UniquePtr<IInspectorWidget>& widget = widgets[i];
        if (widget == nullptr) {
            continue;
        }
        const bool relevant = widget->IsRelevantFor(target);
        widget->SetSectionVisible(relevant);
        if (commitPendingEdits) {
            widget->CommitPendingEdits(widgetCtx);
        }
        if (syncFromTarget && relevant) {
            widget->SyncFromTarget(widgetCtx);
        } else if (commitPendingEdits && !relevant) {
            widget->CancelPendingEdits();
        }
    }
}

void InspectorPanel::OnTick(const FrameTiming& /*timing*/, EditorContext& ctx) {
    if (!built || selection == nullptr) {
        return;
    }

    mode = ctx.mode;
    commandStack = ctx.commandStack;
    engine = ctx.engine;
    viewport = ctx.viewport;

    const InspectorWidgetContext widgetCtx = BuildWidgetContext(ctx);

    if (objectNameField != nullptr && objectNameField->IsEditing() && widgetCtx.target != nullptr) {
        if (!nameEditTracker.HasPendingEdit()) {
            nameEditBefore = widgetCtx.target->GetName();
            nameEditTracker.BeginEdit(nameEditBefore);
        }
        nameEditTracker.MarkChanged();
    }
}

}  // namespace Spark::Editor
