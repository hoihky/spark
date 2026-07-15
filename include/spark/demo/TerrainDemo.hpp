#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"

#include <cstdio>

namespace Spark {

class TerrainDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    void AddPointLight(
            Spark::GameWorld& w,
            Spark::Vector3 position,
            Spark::Vector3 color,
            float intensity,
            float range);


    Spark::Array<Spark::GameObject*> roots{};
    Spark::FlyCamera camera{};
    Spark::SharedPtr<Spark::Texture2D> groundTex;
    Spark::SharedPtr<Spark::Mesh> unitCubeAsset;
    Spark::GameObject* terrainObject = nullptr;
    Spark::TerrainComponent* terrainComp = nullptr;
    Spark::GameObject* editCursorObject = nullptr;
    Spark::TransformComponent* editCursorTransform = nullptr;
    Spark::MaterialComponent* editCursorMaterial = nullptr;
    Spark::GameObject* markerObject = nullptr;
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    float fpsSmoothed = 0.0F;

};

}  // namespace Spark
