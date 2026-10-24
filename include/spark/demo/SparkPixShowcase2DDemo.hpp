#pragma once

#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/SparkPixAtlasLoad.hpp"
#include "spark/scene/camera/Camera2D.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/Scene.hpp"

namespace Spark {

class IEngineContext;

/** Plays spark-pix generated atlases loaded from <c>*.spark-atlas.json</c>. */
class SparkPixShowcase2DDemo {
public:
    void Load(GameWorld& world, IEngineContext& context);
    void Unload(GameWorld& world);
    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    Array<GameObject*> roots{};
    Camera2D camera{};
    DemoHelpHud helpHud{};
    SparkPixAtlasManifest slimeManifest{};
    Utf8String slimeManifestPath{};
    SharedPtr<Texture2D> slimeTex{};
    SpriteAnimatorComponent* slimeAnim = nullptr;
    std::uint32_t clipIndex = 0;
};

}  // namespace Spark
