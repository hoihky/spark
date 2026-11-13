# IGame and Game

`IGame` is the **only** extension point the engine calls each frame. `Game` adds a default `Scene` + `GameWorld` and a stock `OnUpdate` that ticks components and sound cues — you override hooks but rarely reimplement the world container.

## Contract: `IGame`

```cpp
class IGame {
public:
    virtual void OnAttach(IEngineContext& context) = 0;
    virtual void OnDetach() = 0;
    virtual void OnUpdate(const FrameTiming& timing, IEngineContext& context) = 0;
    virtual void OnRender(IRenderFrame& frame, IEngineContext& context) = 0;
};
```

| Hook | Typical work |
|------|--------------|
| `OnAttach` | Spawn entities, load assets, capture cursor, set gravity |
| `OnUpdate` | Input, physics, AI, rules (after `Game::OnUpdate` if using base) |
| `OnRender` | Submit scene (+ GUI paint + ImGui) |
| `OnDetach` | Destroy tracked roots, release subscriptions |

## Base class: `Game`

```cpp
class Game : public IGame {
public:
    Scene& GetScene() noexcept;
    GameWorld& GetWorld() noexcept;

    void OnUpdate(const FrameTiming& timing, IEngineContext& context) override;
    // Default: world.UpdateGameObjects + ProcessSoundCues
};
```

## Concept: minimal lit 3D game

### C++

```cpp
class LitCubeGame final : public Spark::Game {
    FlyCamera camera{};
public:
    void OnAttach(IEngineContext&) override {
        auto& world = GetWorld();
        auto cube = MakeShared<Mesh>(Mesh::CreateUnitCube());
        world.Assets().RegisterMesh(cube, "demo/cube");
        GameObject* go = world.CreateGameObject();
        go->AddComponent<TransformComponent>();
        go->AddComponent<MeshComponent>(cube, SceneMeshSlot::Custom, Vector3{0.7F, 0.3F, 0.2F});
        go->AddComponent<MaterialComponent>(nullptr)->SetRoughness(0.4F);
    }

    void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
        camera.ProcessMovement(ctx.GetInput(), t.deltaTimeSeconds);
        Game::OnUpdate(t, ctx);
    }

    void OnRender(IRenderFrame&, IEngineContext& ctx) override {
        int w = 1, h = 1;
        ctx.GetFramebufferSize(w, h);
        const float aspect = static_cast<float>(w) / static_cast<float>(h);
        const Matrix4 vp = Matrix4::PerspectiveVulkan(DegreesToRadians(70.0F), aspect, 0.1F, 200.0F)
                         * camera.ViewMatrix();
        Vector3 pr{}, pu{};
        camera.BillboardBasis(pr, pu);
        SubmitStandardLitSceneFromWorld(GetWorld(), ctx, vp, camera.position,
            Vector3{0.3F, -1.0F, 0.2F}.Normalized(),
            Vector3{1.0F, 0.98F, 0.95F}, 1.2F, Vector3{0.15F, 0.17F, 0.22F},
            false, pr, pu, 0.0F);
    }
};
```

### C\#

```csharp
public sealed class LitCubeGame : Game
{
    private readonly FlyCamera _camera = new();

    public override void OnAttach(IEngineContext ctx)
    {
        var cube = Mesh.CreateUnitCube();
        World.RegisterMesh(cube, "demo/cube");
        var go = World.CreateGameObject();
        go.GetOrAddTransform();
        go.AddMesh(SceneMeshSlot.Custom, new Vector3(0.7f, 0.3f, 0.2f));
        go.AddMaterial(null).SetRoughness(0.4f);
    }

    public override void OnUpdate(FrameTiming t, IEngineContext ctx)
    {
        _camera.ProcessMovement(ctx.GetInput(), t.DeltaTimeSeconds);
        base.OnUpdate(t, ctx);
    }

    public override void OnRender(IRenderFrame frame, IEngineContext ctx)
    {
        // Build vp from _camera — same as C++ sample
        SceneSubmit.SubmitStandardLitSceneFromWorld(World, ctx, vp, _camera.Position,
            sunDir, sunColor, 1.2f, ambient, false, pr, pu, 0f);
    }
}
```

### Python

```python
class LitCubeGame(Game):
    def on_attach(self, context: EngineContext) -> None:
        world = context.try_get_scene().get_world()
        go = world.create_game_object("Cube")
        go.get_or_add_transform()
        go.add_mesh_custom(albedo=Vector3(0.7, 0.3, 0.2))
        go.add_material().set_roughness(0.4)
```

Python hosts often use a native render path; extend with `on_render` when the host delegates submit to script.

## Concept: 2D platformer skeleton

Physics runs **after** component updates. Track spawned objects for clean `OnDetach`.

### C++

```cpp
class PlatformerGame final : public Game {
    PhysicsSubsystem physics;
    Array<GameObject*> roots;

    void OnAttach(IEngineContext&) override {
        MountUiFont(GetWorld());
        physics.GetWorld2D().GetSettings().gravityY = -32.0F;
        GameObject* player = GetWorld().CreateGameObject();
        roots.PushBack(player);
        player->AddComponent<TransformComponent>();
        player->AddComponent<Rigidbody2DComponent>();
        player->AddComponent<BoxCollider2DComponent>();
        // ground, camera, input maps…
    }

    void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
        Game::OnUpdate(t, ctx);
        physics.Simulate2D(GetWorld(), t);
    }

    void OnRender(IRenderFrame&, IEngineContext& ctx) override {
        SubmitStandardLitSceneFromWorldWithCamera(GetWorld(), ctx,
            sunDir, sunColor, 1.0F, ambient, false, sceneTime);
    }

    void OnDetach() override {
        for (GameObject* go : roots)
            if (go) GetWorld().DestroyGameObject(go);
        roots.Clear();
    }
};
```

### C\#

```csharp
public override void OnUpdate(FrameTiming t, IEngineContext ctx)
{
    base.OnUpdate(t, ctx);
    _physics.Simulate2D(World, t);
}
```

### Python

```python
def on_update(self, timing, context):
    super().on_update(timing, context)
    context.physics.simulate_2d(self.world, timing)
```

`SubmitStandardLitSceneFromWorldWithCamera` resolves the main `Camera2DComponent` or `CameraComponent` — add one to the scene when using this shortcut.

## Concept: async glTF loading

### C++

```cpp
world.Assets().RequestGltf("assets/models/Fox.glb");
world.Assets().Pump();
if (world.Assets().IsGltfReady("assets/models/Fox.glb")) {
    SkinnedGltfAsset fox = world.Assets().LoadSkinnedGltf("assets/models/Fox.glb");
    // spawn SkinnedMeshComponent + AnimatorComponent
}
```

### C\#

```csharp
world.RequestGltf("assets/models/Fox.glb");
world.Assets.Pump();
if (world.Assets.IsGltfReady("assets/models/Fox.glb"))
{
    var fox = world.Assets.LoadSkinnedGltf("assets/models/Fox.glb");
}
```

### Python

```python
world.request_gltf("assets/models/Fox.glb")
world.pump_assets()
if world.is_gltf_ready("assets/models/Fox.glb"):
    fox = world.load_skinned_gltf("assets/models/Fox.glb")
```

## Lifecycle checklist

- [ ] Spawn in `OnAttach`, not constructors (world may not exist yet).
- [ ] Destroy external references in `OnDetach` before world teardown.
- [ ] Never store raw `GameObject*` without tracking — use ids or `Array<GameObject*> roots`.
- [ ] Split **simulation** (`OnUpdate`) from **render** (`OnRender`).

Next: [ECS and Scene](06-ecs-and-scene.md).
