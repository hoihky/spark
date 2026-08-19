# Project Setup

Create a new `Game` subclass and link against `SparkEngine`. Reference `CharacterCameraDemo` and `PhysicsBallThrow3DDemo` in `src/spark/demo/` for working 3D setups.

## OnAttach Essentials

```cpp
void FpsGame::OnAttach(IEngineContext& context) {
    MountUiFont(GetWorld());
    context.GetInput().SetCursorCaptured(true);

    unitCube = MakeShared<Mesh>(Mesh::CreateUnitCube());
    groundMesh = MakeShared<Mesh>(Mesh::CreateGroundPlane(24.0F));
    GetWorld().RegisterMesh(unitCube, "fps/unit_cube");
    GetWorld().RegisterMesh(groundMesh, "fps/ground");

    SpawnArena(GetWorld());
    camera.position = {0.0F, 1.7F, 8.0F};
    camera.moveSpeed = 8.0F;
    camera.mouseSensitivity = 0.12F;
}
```

## CMake Target

```cmake
add_executable(MyFps src/main.cpp src/FpsGame.cpp)
target_link_libraries(MyFps PRIVATE SparkEngine)
target_compile_features(MyFps PRIVATE cxx_std_23)
```

## Mesh Registration Pattern

Always register custom meshes before `MeshComponent` references them:

```cpp
world.RegisterMesh(mesh, "unique/key");
go->AddComponent<MeshComponent>(mesh, SceneMeshSlot::Custom, albedo);
```

## Physics (Optional)

If using thrown objects or character controllers:

```cpp
PhysicsSubsystem physics;

void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    // movement + shooting ...
    Game::OnUpdate(t, ctx);
    physics.SimulateAll3D(GetWorld(), t);
}
```

See `PhysicsBallThrow3DDemo` for rigidbody spheres and `CharacterCameraDemo` for `CharacterController3DComponent`.

Next: [Arena and Targets](03-arena.md).
