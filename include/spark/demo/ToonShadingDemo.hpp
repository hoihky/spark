#pragma once

#include "spark/demo/DemoMode.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"

namespace Spark {

/**
 * Small lit scene comparing <c>SceneShadingModel::LitPbr</c> vs <c>ToonCel</c> on <c>MaterialComponent</c>.
 * Fly camera (F1 mouse lock); keys <c>[</c> / <c>]</c> cycle diffuse bands on the front toon column.
 */
class ToonShadingDemo {
public:
    void Load(GameWorld& w, IEngineContext& context);
    void Unload(GameWorld& w);
    void Simulate(const FrameTiming& timing, IEngineContext& context);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    Array<GameObject*> roots{};
    FlyCamera camera{};

    SharedPtr<Mesh> unitCube{};
    SharedPtr<Mesh> groundMesh{};

    GameObject* helpHud = nullptr;
    TextOverlayComponent* helpText = nullptr;

    GameObject* toonBandsDemoCube = nullptr;
    MaterialComponent* toonBandsMaterial = nullptr;

    float spinRadians = 0.0F;
};

}  // namespace Spark
