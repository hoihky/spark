# Project Setup

Create a `Game` subclass and link `SparkEngine`. Copy patterns from `CharacterCameraDemo` and `PhysicsBallThrow3DDemo` under `src/spark/demo/`.

## Concept: CMake target

### C++

```cmake
add_executable(MyFps src/main.cpp src/FpsGame.cpp)
target_link_libraries(MyFps PRIVATE SparkEngine)
target_compile_features(MyFps PRIVATE cxx_std_23)
```

### Shell (spark-sharp)

```bash
cd ../spark-sharp
cmake --preset debug -DSPARK_ENGINE_ROOT=/path/to/spark
cmake --build build --target SparkRunHelloCsGameDebug
```

### Python

```bash
pip install -e ../spark-python
export SPARK_ENGINE_ROOT=/path/to/spark
python -m spark samples.hello_game
```

## Concept: `OnAttach` essentials

### C++

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

### C\#

```csharp
public override void OnAttach(IEngineContext context)
{
    World.MountUiFont();
    context.GetInput().SetCursorCaptured(true);
    World.RegisterMesh(Mesh.CreateUnitCube(), "fps/unit_cube");
    SpawnArena();
    _camera.Position = new Vector3(0f, 1.7f, 8f);
}
```

### Python

```python
def on_attach(self, context):
    context.get_input().set_cursor_captured(True)
    self.world.register_unit_cube_mesh("fps/unit_cube")
    self._spawn_arena()
```

## Concept: mesh registration

Register meshes **before** `MeshComponent` references them by key or shared pointer.

### C++

```cpp
world.Assets().RegisterMesh(mesh, "unique/key");
go->AddComponent<MeshComponent>(mesh, SceneMeshSlot::Custom, albedoTint);
```

### C\#

```csharp
world.Assets().RegisterMesh(cube, "fps/unit_cube");
go.AddMesh(SceneMeshSlot.Custom, new Vector3(0.8f, 0.6f, 0.4f));
```

### Python

```python
go.get_or_add_default_component(ComponentKind.Mesh)
# Prefer generated add_mesh when bound; else attach via native template host
```

## Concept: optional 3D physics

If you add thrown balls or `CharacterController3D`:

### C++

```cpp
PhysicsSubsystem physics;

void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    TickFpsCamera(t, ctx);
    Game::OnUpdate(t, ctx);
    physics.SimulateAll3D(GetWorld(), t);
}
```

### C\#

```csharp
_physics.SimulateAll3D(World, timing);
```

### Python

```python
context.physics.simulate_all_3d(self.world, timing)
```

See `PhysicsShowcase3DDemo` (**P**) for the recommended integration order (layers, queries, carry).

Next: [Arena and Targets](03-arena.md).
