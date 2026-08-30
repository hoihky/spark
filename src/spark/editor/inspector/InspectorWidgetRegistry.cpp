#include "spark/editor/inspector/InspectorWidgetRegistry.hpp"

#include "spark/editor/inspector/widgets/MaterialInspectorWidget.hpp"
#include "spark/editor/inspector/widgets/MeshInspectorWidget.hpp"
#include "spark/editor/inspector/widgets/ParticleEmitterInspectorWidget.hpp"
#include "spark/editor/inspector/widgets/PointLightInspectorWidget.hpp"
#include "spark/editor/inspector/widgets/TransformInspectorWidget.hpp"
#include "spark/editor/inspector/widgets/VfxPlayerInspectorWidget.hpp"

namespace Spark::Editor {

namespace {

void RegisterWidget(Array<UniquePtr<IInspectorWidget>>& widgets, UniquePtr<IInspectorWidget> widget) {
    widgets.PushBack(MoveTemp(widget));
}

template<typename TWidget>
void RegisterWidgetType(Array<UniquePtr<IInspectorWidget>>& widgets) {
    auto concrete = MakeUnique<TWidget>();
    RegisterWidget(widgets, UniquePtr<IInspectorWidget>(concrete.Release()));
}

}  // namespace

InspectorWidgetRegistry::InspectorWidgetRegistry() {
    RegisterWidgetType<TransformInspectorWidget>(widgets);
    RegisterWidgetType<MeshInspectorWidget>(widgets);
    RegisterWidgetType<PointLightInspectorWidget>(widgets);
    RegisterWidgetType<MaterialInspectorWidget>(widgets);
    RegisterWidgetType<ParticleEmitterInspectorWidget>(widgets);
    RegisterWidgetType<VfxPlayerInspectorWidget>(widgets);
}

void InspectorWidgetRegistry::PrepareFrameContext(const InspectorWidgetContext& ctx) noexcept {
    for (std::size_t i = 0; i < widgets.GetSize(); ++i) {
        if (widgets[i] != nullptr) {
            widgets[i]->PrepareFrameContext(ctx);
        }
    }
}

bool InspectorWidgetRegistry::HasAnyPendingEdits() const noexcept {
    for (std::size_t i = 0; i < widgets.GetSize(); ++i) {
        if (widgets[i] != nullptr && widgets[i]->HasPendingEdits()) {
            return true;
        }
    }
    return false;
}

}  // namespace Spark::Editor
