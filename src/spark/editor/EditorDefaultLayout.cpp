#include "spark/editor/EditorDefaultLayout.hpp"

#include "spark/ui/runtime/EditorLayoutStore.hpp"

namespace Spark::Editor {

void EditorDefaultLayout::ApplyTo(Ui::SceneEditorLayoutSettings& settings) noexcept {
    settings.leftDockWidthPx = kLeftWidthPx;
    settings.rightDockWidthPx = kRightWidthPx;
    settings.leftStackSplit = kLeftStackSplit;
    settings.sidebarWidthPx = kLeftWidthPx;
    Ui::SetSceneEditorSidebarWidthPx(kLeftWidthPx);
    Ui::SetSceneEditorSidebarSplit(kLeftStackSplit);
}

}  // namespace Spark::Editor
