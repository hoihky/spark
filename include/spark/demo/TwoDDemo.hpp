#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"

namespace Spark {

class TwoDDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    Spark::Array<Spark::GameObject*> roots{};
    Spark::Camera2D camera{};
    Spark::SharedPtr<Spark::Texture2D> checkerTex{};
    Spark::GameObject* mapObject = nullptr;
    Spark::GameObject* spriteRed = nullptr;
    Spark::GameObject* spriteGreen = nullptr;
    Spark::TransformComponent* spriteRedTr = nullptr;
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    float fpsSmoothed = 0.0F;

};

/** Side-scrolling platformer: BoxCollider2D static solids + Rigidbody2D dynamic player, Camera2D follow. */

}  // namespace Spark
