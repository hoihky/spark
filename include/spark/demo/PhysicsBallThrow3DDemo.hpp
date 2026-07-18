#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/demo/ShellDemoUi.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/ui/GuiCanvasComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SpringJoint3DComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/gui/GuiControls.hpp"
#include "spark/gui/GuiScene.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/Scene.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Spark {

/** Right-hand tuning strip so the 3D view stays interactive on the left (same idea as particle demo). */
class PhysicsBallGuiDockRoot final : public Gui::Widget {
public:
    void Arrange(const Gui::Rect& r) override {
        constexpr float kMargin = 14.0F;
        constexpr float kMinW = 380.0F;
        constexpr float kMaxW = 540.0F;
        const float wMax = (std::max)(kMinW, r.width - 2.0F * kMargin);
        const float w = std::clamp(r.width * 0.36F, kMinW, (std::min)(kMaxW, wMax));
        const float h = (std::max)(340.0F, r.height - 2.0F * kMargin);
        const float x = r.x + (std::max)(0.0F, r.width - w - kMargin);
        const float y = r.y + kMargin;
        const Gui::Rect panel{x, y, w, h};
        bounds = panel;
        for (std::size_t i = 0; i < children.GetSize(); ++i) {
            if (children[i]) {
                children[i]->Arrange(panel);
            }
        }
    }
};

/**
 * First-person fly camera, static ground + boxes, one dynamic sphere, and <c>PhysicsMaterial3DComponent</c> on
 * surfaces. Defaults use ~1 m = 1 unit, <c>g ≈ 9.81 m/s²</c> downward (+Y up), and SI-style masses for a plausible
 * throw. Uses <c>SimulatePhysics3D</c> with substeps.
 */
class PhysicsBallThrow3DDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);


    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    void ClearGuiRefs() noexcept;


    void BuildPhysTuningPanel(Spark::GuiCanvasComponent& canvas);


    void ApplyTuningFromGui() noexcept;


    void ApplyCubeBounciness(const float tIn) noexcept;


    void ApplyCubeMassScale(const float massKg) noexcept;


    Spark::Array<Spark::GameObject*> roots{};
    Spark::Array<Spark::GameObject*> cubeObjects{};
    Spark::Array<bool> cubeRubber{};
    Spark::FlyCamera camera{};
    Spark::SharedPtr<Spark::Mesh> skyMesh;
    Spark::SharedPtr<Spark::Mesh> groundMesh;
    Spark::SharedPtr<Spark::Mesh> cubeMesh;
    Spark::SharedPtr<Spark::Mesh> ballMesh;
    Spark::Rigidbody3DComponent* ballRb = nullptr;
    Spark::TransformComponent* ballTr = nullptr;
    Spark::Rigidbody3DComponent* pendulumBobRb = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    Spark::GameObject* guiCanvasGo = nullptr;
    Spark::Gui::Slider* guiGravity = nullptr;
    Spark::Gui::Slider* guiBallMass = nullptr;
    Spark::Gui::Slider* guiThrow = nullptr;
    Spark::Gui::Slider* guiCubeBounce = nullptr;
    Spark::Gui::Slider* guiCubeMass = nullptr;
    float gravityY = -9.81F;
    float throwSpeed = 12.0F;
    float fpsSmoothed = 0.0F;

};

}  // namespace Spark
