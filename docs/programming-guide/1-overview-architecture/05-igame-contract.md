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
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"

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

## 2D Platformer Pattern (from `Platformer2DDemo`)

Most SparkDemo games split setup across **Load / Simulate / Render** helpers. A minimal 2D game using the same ideas:

```cpp
class PlatformerGame final : public Spark::Game {
    Spark::PhysicsSubsystem physics;
    Spark::Array<Spark::GameObject*> roots;

public:
    void OnAttach(Spark::IEngineContext& ctx) override {
        Spark::MountUiFont(GetWorld());  // required for TextOverlayComponent
        auto& w = GetWorld();

        physics.GetWorld2D().GetSettings().gravityY = -32.0F;

        // Player
        auto* player = w.CreateGameObject();
        roots.PushBack(player);
        player->AddComponent<Spark::TransformComponent>()->SetTranslation({0, 0, 0});
        player->AddComponent<Spark::Rigidbody2DComponent>();
        player->AddComponent<Spark::BoxCollider2DComponent>();

        // Ground platform
        auto* ground = w.CreateGameObject();
        roots.PushBack(ground);
        auto* gtr = ground->AddComponent<Spark::TransformComponent>();
        gtr->SetTranslation({0, -1.5F, 0});
        gtr->SetScale({20.0F, 1.0F, 1.0F});
        ground->AddComponent<Spark::BoxCollider2DComponent>();
    }

    void OnUpdate(const Spark::FrameTiming& t, Spark::IEngineContext& ctx) override {
        Spark::Game::OnUpdate(t, ctx);  // ticks components + sound cues
        physics.Simulate2D(GetWorld(), t);
    }

    void OnRender(Spark::IRenderFrame&, Spark::IEngineContext& ctx) override {
        Spark::SubmitStandardLitSceneFromWorldWithCamera(
            GetWorld(), ctx,
            Spark::Vector3{0.3F, -0.9F, 0.2F}.Normalized(),
            Spark::Vector3{1.0F, 0.98F, 0.95F}, 1.0F,
            Spark::Vector3{0.15F, 0.17F, 0.22F},
            false, 0.0F);
    }

    void OnDetach() override {
        for (Spark::GameObject* go : roots)
            if (go) GetWorld().DestroyGameObject(go);
        roots.Clear();
    }
};
```

`SubmitStandardLitSceneFromWorldWithCamera` resolves the main `Camera2DComponent` or `CameraComponent` in the world — add one to the scene if you use this shortcut.

## Asset Loading

Register meshes and textures with cache keys before attaching components:

```cpp
auto mesh = Spark::MakeShared<Spark::Mesh>(Spark::Mesh::CreateUnitCube());
world.RegisterMesh(mesh, "my_game/cube");

auto tex = Spark::MakeShared<Spark::Texture2D>(/* ... */);
world.RegisterTexture(tex, "my_game/tiles");

// Async glTF (preferred for large models):
world.RequestGltf("assets/models/Fox.glb");
// Each frame until ready:
world.PumpAssets();
if (world.IsGltfReady("assets/models/Fox.glb")) {
    Spark::GltfAsset asset = world.LoadGltf("assets/models/Fox.glb");
    // spawn SkinnedMeshComponent + AnimatorComponent ...
}
```

Next: [ECS and Scene](06-ecs-and-scene.md).
