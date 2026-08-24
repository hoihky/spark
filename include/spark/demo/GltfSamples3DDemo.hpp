#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"

namespace Spark {

class TextOverlayComponent;

/** Fly-camera scene displaying Khronos DamagedHelmet.glb. */
class GltfSamples3DDemo {
public:
    void Load(GameWorld& w, IEngineContext& context);
    void Unload(GameWorld& w);
    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    bool TryPlaceRigidGltf(
            GameWorld& w,
            const char* relativePath,
            const Vector3& pos,
            float yawRadians,
            float targetMaxExtentM);

    Array<GameObject*> roots{};
    FlyCamera camera{};

    SharedPtr<Mesh> groundMesh{};

    bool helmetLoaded = false;

    GameObject* helpHud = nullptr;
    TextOverlayComponent* helpText = nullptr;
};

}  // namespace Spark
