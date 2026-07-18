---
title: IGame and Game
order: 5
---

# IGame and Game

## Class Design: `IGame`

`IGame` is the **application contract**. `Engine` calls four virtual hooks:

```cpp
class IGame {
public:
    virtual ~IGame() = default;
    virtual void OnAttach(IEngineContext& context) = 0;
    virtual void OnDetach() = 0;
    virtual void OnUpdate(const FrameTiming& timing, IEngineContext& context) = 0;
    virtual void OnRender(IRenderFrame& frame, IEngineContext& context) = 0;
};
```

| Hook | Typical work |
|------|--------------|
| `OnAttach` | Spawn entities, load assets, configure window |
| `OnUpdate` | Input, physics, AI, gameplay rules |
| `OnRender` | Build `SceneRenderParams`, submit to context |
| `OnDetach` | Destroy tracked objects, release handles |

## Class Design: `Game`

`Spark::Game` provides a convenience base:

```cpp
class Game : public IGame {
public:
    Scene& GetScene() noexcept;
    GameWorld& GetWorld() noexcept;

    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override;
    // Default: world.UpdateGameObjects + ProcessSoundCues
};
```

`Game` owns a `Scene` façade over its internal `GameWorld`.

## Full Minimal 3D Example

```cpp
#include "spark/engine/Game.hpp"
#include "spark/scene/SceneSubmit.hpp"
#include "spark/scene/FlyCamera.hpp"
#include "spark/ecs/components/MeshComponent.hpp"
#include "spark/ecs/components/MaterialComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"

class LitCubeGame final : public Spark::Game {
    Spark::FlyCamera camera{};

public:
    void OnAttach(Spark::IEngineContext& ctx) override {
        auto& world = GetWorld();
        auto cube = Spark::MakeShared<Spark::Mesh>(Spark::Mesh::CreateUnitCube());
        world.RegisterMesh(cube, "demo/cube");

        auto* go = world.CreateGameObject();
        go->AddComponent<Spark::TransformComponent>();
        go->AddComponent<Spark::MeshComponent>(cube, Spark::SceneMeshSlot::Custom, Spark::Vector3{0.7F, 0.3F, 0.2F});
        go->AddComponent<Spark::MaterialComponent>(nullptr)->SetRoughness(0.4F);
    }

    void OnUpdate(const FrameTiming& t, Spark::IEngineContext& ctx) override {
        camera.ProcessMovement(ctx.GetInput(), t.deltaTimeSeconds);
        Game::OnUpdate(t, ctx);
    }

    void OnRender(Spark::IRenderFrame&, Spark::IEngineContext& ctx) override {
        int w = 1, h = 1;
        ctx.GetFramebufferSize(w, h);
        const float aspect = static_cast<float>(w) / static_cast<float>(h);
        const Spark::Matrix4 proj = Spark::Matrix4::PerspectiveVulkan(
            Spark::DegreesToRadians(70.0F), aspect, 0.1F, 200.0F);
        const Spark::Matrix4 vp = proj * camera.ViewMatrix();
        Spark::Vector3 pr{}, pu{};
        camera.BillboardBasis(pr, pu);

        Spark::SubmitStandardLitSceneFromWorld(
            GetWorld(), ctx, vp, camera.position,
            Spark::Vector3{0.3F, -1.0F, 0.2F}.Normalized(),
            Spark::Vector3{1.0F, 0.98F, 0.95F}, 1.2F,
            Spark::Vector3{0.15F, 0.17F, 0.22F},
            false, pr, pu, 0.0F);
    }
};
```

## Lifecycle Best Practice

Track spawned roots in an `Array<GameObject*>` and destroy in `OnDetach`:

```cpp
void OnDetach() override {
    for (GameObject* go : roots)
        if (go) GetWorld().DestroyGameObject(go);
    roots.Clear();
}
```

Next: [ECS and Scene](overview-architecture/06-ecs-and-scene.html).
