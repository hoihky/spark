# Introduction to Spark

Spark is a desktop-focused **C++23** game engine. It combines a GLFW window, Vulkan forward renderer, entity-component scene graph, optional 2D/3D physics, AI modules, audio mixing, a CPU-painted **retained GUI** toolkit, and optional **Dear ImGui** for immediate-mode tools.

Games can ship as **native C++** (`Game` subclasses), **C#** via spark-sharp interop (`Spark.Scripting`), or **Python** via spark-python — all three target the same ECS and `FillStandardLitSceneFromWorld` render contract.

## What You Get

| Layer | Technology | Public API |
|-------|------------|------------|
| Window / input | GLFW | `Window`, `IInput` |
| Rendering | Vulkan (internal) | `SceneRenderParams`, `IFramePresenter` |
| Simulation | ECS (**120+** built-in components) | `GameWorld`, `GameObject`, `GameComponent` |
| Physics | Custom solvers | `PhysicsSubsystem`, `PhysicsWorld2D`, `PhysicsWorld3D` |
| AI | FSM, GOAP, steering | `SimulateGameAi`, `AiAgentComponent` |
| Audio | Software mixer | `SoundEngine`, `SoundCueComponent` |
| UI (retained) | `IUiElement` tree → screen draws | `UiCanvasComponent`, `spark/ui/Ui.hpp` |
| Scripting | Native interop | Component catalog → C# / Python mirrors |

## Design Philosophy

1. **Explicit frame contract** — simulation in `OnUpdate`, rendering as immutable `SceneRenderParams` in `OnRender` (or engine-owned submit for script hosts).
2. **Dependency inversion** — games depend on `IGame` / `IEngineContext`, not `VulkanRenderer`.
3. **Composition** — behavior lives in components; `GameObject` is a lightweight container.
4. **Demos as documentation** — `src/spark/demo/` and `SparkDemo` showcase every subsystem. `Platformer2DDemo` (#6) demonstrates parallax, screen shake, semantic input, character controller, pickups, and game-flow state components end-to-end.

## Concept: minimal bootstrap

### C++

```cpp
#include "spark/engine/Engine.hpp"
#include "spark/engine/Game.hpp"

class HelloSpark final : public Spark::Game {
public:
    void OnAttach(Spark::IEngineContext& ctx) override {
        GameObject* go = GetWorld().CreateGameObject();
        go->GetName() = Spark::Utf8String("Root");
        go->AddComponent<Spark::TransformComponent>();
    }
};

int main() {
    Spark::Engine engine(Spark::Engine::NewGame<HelloSpark>());
    engine.Run();
    return 0;
}
```

### C\#

C# games reference `Spark.Bindings` and subclass the template `Game` type (see `spark-sharp`):

```csharp
public sealed class HelloSpark : Game
{
    public override void OnAttach(IEngineContext ctx)
    {
        var go = World.CreateGameObject("Root");
        go.GetOrAddTransform();
    }
}

// Entry (template): host loads managed assembly and calls Native.spark_* bootstrap.
```

Bindings are generated from `component-script-catalog.json` — not from header scraping.

### Python

```python
from spark import Game
from spark.engine import EngineContext

class HelloGame(Game):
    def on_attach(self, context: EngineContext) -> None:
        scene = context.try_get_scene()
        world = scene.get_world()
        root = world.create_game_object("Root")
        root.get_or_add_transform()

# spark-python host: python -m spark samples.hello_game
```

See `spark-python/samples/hello_game.py` for camera + sprite setup.

## Three languages, one ECS

| Task | C++ | C# | Python |
|------|-----|----|----|
| Create entity | `world.CreateGameObject()` | `world.CreateGameObject(name)` | `world.create_game_object(name)` |
| Transform | `AddComponent<TransformComponent>()` | `GetOrAddTransform()` | `get_or_add_transform()` |
| 2D sprite | `AddComponent<SpriteComponent>(...)` | `AddSprite(key, tint, uv, sort)` | `add_sprite(...)` |
| 3D mesh | `AddComponent<MeshComponent>(...)` | `AddMesh(slot, albedo)` | `add_mesh_*` helpers |

Component methods map 1:1 from the catalog; if a Python name differs slightly, regenerate interop after engine upgrades.

## Default Demo Binary

`src/main.cpp` constructs `Engine` with `NewShellDemoGame()` — an interactive launcher with **21+** built-in modes (3D fly scenes, maze, 2D games, material showcase, glTF PBR, tilemap showcase, ImGui docking demo, and more).

## Where to go next

| Goal | Chapter |
|------|---------|
| Build & run | [Building and Running](03-building-and-running.md) |
| Pick a SparkDemo | [SparkDemo catalog](10-sparkdemo-catalog.md) |
| Frame loop | [Engine Loop](04-engine-loop.md) |
| ECS mental model | [ECS and Scene](06-ecs-and-scene.md) |
| Scenes & saves | [Scenes and saves](11-scenes-and-saves.md) |
| glTF & asset keys | [Assets pipeline](12-assets-and-content-pipeline.md) |
| C\# / Python hosts | [Scripting and bindings](13-scripting-and-bindings.md) |
| Component usage (C++ / C\# / Python) | [Part 7 — Game components usage](../7-game-components-usage/00-introduction.md) |
| Full component API tables | [Game component reference](07-game-component-reference.md) |
| 2D rendering depth | [Sprites](../2-2d-graphics/01-sprites.md) |
| 3D PBR | [Meshes and Materials](../3-3d-graphics/01-meshes-and-materials.md) |

Next: [Engine Capabilities](02-engine-capabilities.md).
