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
    struct LoadedHero {
        SparkPixAtlasManifest manifest{};
        Utf8String manifestPath{};
        SharedPtr<Texture2D> texture{};
        GameObject* object = nullptr;
        TransformComponent* transform = nullptr;
        SpriteAnimatorComponent* animator = nullptr;
        Utf8String label{};
        std::uint32_t idleClipIndex = 0;
        std::uint32_t walkClipIndex = 0;
        std::uint32_t runClipIndex = 0;
        float facingSign = 1.0F;
    };

    Array<GameObject*> roots{};
    Array<LoadedHero> heroes{};
    Camera2D camera{};
    DemoHelpHud helpHud{};
    std::size_t activeHeroIndex = 0;
};

}  // namespace Spark
