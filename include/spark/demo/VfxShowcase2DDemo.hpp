#pragma once

#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/VfxShowcase2DDetail.hpp"
#include "spark/scene/camera/Camera2D.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/Scene.hpp"

namespace Spark {

class IEngineContext;

/** Orthographic playground for built-in sprite-layer VFX presets (`*_2d`). */
class VfxShowcase2DDemo {
public:
    void Load(GameWorld& world, IEngineContext& context);
    void Unload(GameWorld& world);
    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    void PlayAt(GameWorld& world, const Vector3& worldPos);
    [[nodiscard]] bool PickWorldXY(IEngineContext& context, float px, float py, Vector2& outWorld) const;

    Array<GameObject*> roots{};
    Camera2D camera{};
    Vector3 spawnPoint{0.0F, 0.0F, 0.05F};
    int selectedIndex = 0;
    DemoHelpHud helpHud{};
};

}  // namespace Spark
