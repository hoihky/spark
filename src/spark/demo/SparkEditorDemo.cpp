#include "spark/demo/SparkEditorDemo.hpp"

#include "spark/scene/core/Scene.hpp"

namespace Spark {

void SparkEditorDemo::Load(Scene& scene, IEngineContext& context) {
    if (loaded) {
        return;
    }
    editor.OnAttach(scene, context);
    loaded = true;
}

void SparkEditorDemo::Unload(Scene& scene) {
    if (!loaded) {
        return;
    }
    editor.OnDetach(scene);
    loaded = false;
}

void SparkEditorDemo::Simulate(const FrameTiming& timing, Scene& scene, IEngineContext& context) {
    if (!loaded) {
        return;
    }
    editor.OnUpdate(timing, scene, context);
}

void SparkEditorDemo::Render(Scene& scene, IEngineContext& context) {
    if (!loaded) {
        return;
    }
    editor.OnRender(scene, context);
}

}  // namespace Spark
