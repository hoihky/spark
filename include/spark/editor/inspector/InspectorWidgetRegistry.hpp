#pragma once

#include "spark/editor/inspector/IInspectorWidget.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/core/Array.hpp"

namespace Spark::Editor {

/** Factory registry for component inspector strategies. */
class InspectorWidgetRegistry final {
public:
    InspectorWidgetRegistry();
    [[nodiscard]] const Array<UniquePtr<IInspectorWidget>>& GetWidgets() const noexcept { return widgets; }
    void PrepareFrameContext(const InspectorWidgetContext& ctx) noexcept;
    [[nodiscard]] bool HasAnyPendingEdits() const noexcept;

private:
    Array<UniquePtr<IInspectorWidget>> widgets{};
};

}  // namespace Spark::Editor
