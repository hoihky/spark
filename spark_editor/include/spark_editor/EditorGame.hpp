#pragma once

#include "spark/engine/Game.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

namespace Editor {
class EditorApplication;
}

/** Spark Editor entry — thin IGame wrapper around EditorApplication. */
class EditorGame final : public Game {
public:
    EditorGame();
    ~EditorGame() override;

    void OnAttach(IEngineContext& context) override;
    void OnDetach() override;
    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override;
    void OnRender(IRenderFrame& frame, IEngineContext& context) override;

private:
    UniquePtr<Editor::EditorApplication> editor;
};

[[nodiscard]] UniquePtr<IGame> NewEditorGame();

}  // namespace Spark
