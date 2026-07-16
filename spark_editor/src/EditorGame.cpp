#include "spark_editor/EditorGame.hpp"

#include "spark/editor/EditorApplication.hpp"
#include "spark/engine/IGame.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

EditorGame::EditorGame() : editor(MakeUnique<Editor::EditorApplication>()) {}

EditorGame::~EditorGame() = default;

void EditorGame::OnAttach(IEngineContext& context) {
    editor->OnAttach(GetScene(), context);
}

void EditorGame::OnDetach() {
    editor->OnDetach(GetScene());
}

void EditorGame::OnUpdate(const FrameTiming& timing, IEngineContext& context) {
    Game::OnUpdate(timing, context);
    editor->OnUpdate(timing, GetScene(), context);
}

void EditorGame::OnRender(IRenderFrame& /*frame*/, IEngineContext& context) {
    editor->OnRender(GetScene(), context);
}

UniquePtr<IGame> NewEditorGame() {
    return UniquePtr<IGame>(new EditorGame());
}

}  // namespace Spark
