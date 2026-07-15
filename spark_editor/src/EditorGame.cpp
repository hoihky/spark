#include "spark_editor/EditorGame.hpp"

#include "spark/editor/EditorApplication.hpp"
#include "spark/engine/IGame.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

EditorGame::EditorGame() : editor_(MakeUnique<Editor::EditorApplication>()) {}

EditorGame::~EditorGame() = default;

void EditorGame::OnAttach(IEngineContext& context) {
    editor_->OnAttach(GetScene(), context);
}

void EditorGame::OnDetach() {
    editor_->OnDetach(GetScene());
}

void EditorGame::OnUpdate(const FrameTiming& timing, IEngineContext& context) {
    Game::OnUpdate(timing, context);
    editor_->OnUpdate(timing, GetScene(), context);
}

void EditorGame::OnRender(IRenderFrame& /*frame*/, IEngineContext& context) {
    editor_->OnRender(GetScene(), context);
}

UniquePtr<IGame> NewEditorGame() {
    return UniquePtr<IGame>(new EditorGame());
}

}  // namespace Spark
