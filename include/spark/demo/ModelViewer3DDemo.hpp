#pragma once

#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"

namespace Spark {

/** Fly-camera viewer that cycles bundled glTF models with TAB. */
class ModelViewer3DDemo {
public:
    void Load(GameWorld& w, IEngineContext& context);
    void Unload(GameWorld& w);
    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    void ShowModel(GameWorld& w, std::size_t index);
    void AdvanceModel(GameWorld& w, int delta);
    void RefreshHud() noexcept;
    void FrameCamera(float targetExtentM) noexcept;

    Array<GameObject*> sceneRoots{};
    GameObject* modelPivot = nullptr;

    FlyCamera camera{};
    SharedPtr<Mesh> groundMesh{};
    SharedPtr<Mesh> skyMesh{};
    SharedPtr<Texture2D> envEquirectTex{};
    bool envHdrLoaded = false;

    std::size_t currentIndex = 0;
    bool currentLoadOk = false;
    DemoHelpHud helpHud{};
};

}  // namespace Spark
