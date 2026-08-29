#pragma once

#include "spark/editor/EditorApplication.hpp"
#include "spark/editor/EditorTypes.hpp"

namespace Spark {

class IEngineContext;
class Scene;
struct FrameTiming;

/**
 * Shell-friendly wrapper around <c>EditorApplication</c> (Load / Unload / Simulate / Render).
 */
class SparkEditorDemo final {
public:
    void Load(Scene& scene, IEngineContext& context);
    void Unload(Scene& scene);
    void Simulate(const FrameTiming& timing, Scene& scene, IEngineContext& context);
    void Render(Scene& scene, IEngineContext& context);

    [[nodiscard]] bool IsPlayActive() const noexcept {
        return editor.GetMode() == Editor::EditorMode::Play;
    }

private:
    Editor::EditorApplication editor{};
    bool loaded = false;
};

}  // namespace Spark
