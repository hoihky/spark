#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"

namespace Spark {

class ThreeDDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
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
