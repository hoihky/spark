#pragma once

#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"

namespace Spark {

class FoliageWindDemo {
public:
    void Load(GameWorld& world, IEngineContext& context);
    void Unload(GameWorld& world);
    void Simulate(const FrameTiming& timing, IEngineContext& context);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    FlyCamera camera{};
    float sceneTime = 0.0F;
    DemoHelpHud helpHud{};
    Array<GameObject*> roots{};
    GameObject* windEnvironmentObject = nullptr;
};

}  // namespace Spark
