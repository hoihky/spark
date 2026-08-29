#pragma once

#include "spark/demo/SparkEditorDemo.hpp"
#include "spark/engine/Game.hpp"

namespace Spark {

/** Standalone <c>IGame</c> entry for the <c>SparkEditor</c> executable. */
class EditorGame final : public Game {
public:
    void OnAttach(IEngineContext& context) override;
    void OnDetach() override;
    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override;
    void OnRender(IRenderFrame& frame, IEngineContext& context) override;

private:
    SparkEditorDemo editorDemo{};
};

[[nodiscard]] UniquePtr<IGame> NewEditorGame();

}  // namespace Spark
