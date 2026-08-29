#include "spark/editor/EditorGame.hpp"

#include "spark/demo/DemoGuiFrame.hpp"
#include "spark/engine/Engine.hpp"
#include "spark/ui/Ui.hpp"
#include "spark/ui/runtime/UiScene.hpp"

namespace Spark {

void EditorGame::OnAttach(IEngineContext& context) {
    Spark::Ui::SceneEditorLayoutSettings layout{};
    (void)Spark::Ui::TryLoadSceneEditorLayout(layout);
    Spark::Ui::SetActiveUiThemePreset(layout.guiTheme);
    DemoGui::ActivateDearImGuiDemoUi(context);
    GetScene().SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);
    context.GetInput().SetCursorCaptured(false);
    editorDemo.Load(GetScene(), context);
}

void EditorGame::OnDetach() {
    editorDemo.Unload(GetScene());
}

void EditorGame::OnUpdate(const FrameTiming& timing, IEngineContext& context) {
    editorDemo.Simulate(timing, GetScene(), context);
    Game::OnUpdate(timing, context);
}

void EditorGame::OnRender(IRenderFrame& /*frame*/, IEngineContext& context) {
    editorDemo.Render(GetScene(), context);
}

UniquePtr<IGame> NewEditorGame() {
    return Engine::NewGame<EditorGame>();
}

}  // namespace Spark
