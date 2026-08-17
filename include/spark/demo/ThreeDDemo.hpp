#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"

#include <cstdint>

namespace Spark {

class GameWorld;

class ThreeDDemo {
public:
    /** Queue glTF assets for background load (safe to call from the launcher before entering the demo). */
    static void RequestGltfAssets(Spark::GameWorld& world) noexcept;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    enum class PendingGltfKind : std::uint8_t { Hero, Chair, Fox };

    struct PendingGltfLoad {
        Spark::Utf8String path;
        PendingGltfKind kind = PendingGltfKind::Hero;
        bool spawned = false;
    };

    void PollPendingGltfLoads(Spark::GameWorld& w);
    void SpawnHero(Spark::GameWorld& w, const Spark::GltfAsset& asset, const Spark::Utf8String& path);
    void SpawnChair(Spark::GameWorld& w, const Spark::GltfAsset& asset);
    void SpawnFox(Spark::GameWorld& w, const Spark::SkinnedGltfAsset& asset);

    Spark::Array<PendingGltfLoad> pendingGltfLoads{};
    Spark::GameWorld* loadedWorld = nullptr;
    Spark::Array<Spark::GameObject*> roots{};

    Spark::FlyCamera camera;
    float cubeYawRadians = 0.0F;

    Spark::SharedPtr<Spark::Mesh> unitCubeAsset;
    Spark::SharedPtr<Spark::Mesh> heroMeshAsset;
    Spark::SharedPtr<Spark::Mesh> chairMeshAsset;
    Spark::SharedPtr<Spark::Mesh> groundAsset;
    Spark::SharedPtr<Spark::Texture2D> checkerTex;
    Spark::SharedPtr<Spark::Texture2D> brickTex;
    Spark::GameObject* groundObject = nullptr;
    Spark::GameObject* cubeObject = nullptr;
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    float fpsSmoothed = 0.0F;

};

}  // namespace Spark
