# Project Setup

```bash
cp -r samples/fps_game_template my_fps
```

## OnAttach Essentials

```cpp
void FpsGame::OnAttach(IEngineContext& context) {
    MountUiFontIfNeeded(GetWorld());
    context.GetInput().SetCursorCaptured(true);
    glfwSetWindowTitle(context.GetWindow().Handle(), "My FPS");

    unitCube = MakeShared<Mesh>(Mesh::CreateUnitCube());
    groundMesh = MakeShared<Mesh>(Mesh::CreateGroundPlane(24.0F));
    GetWorld().RegisterMesh(unitCube, "fps/unit_cube");
    GetWorld().RegisterMesh(groundMesh, "fps/ground");

    SpawnArena(GetWorld());
}
```

## Mesh Registration Pattern

Always register custom meshes before `MeshComponent` references them:

```cpp
world.RegisterMesh(mesh, "unique/key");
go->AddComponent<MeshComponent>(mesh, SceneMeshSlot::Custom, albedo);
```

Next: [Arena and Targets](03-arena.md).
