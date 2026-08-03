# Introduction to Spark

Spark is a desktop-focused **C++23** game engine. It combines a GLFW window, Vulkan forward renderer, entity-component scene graph, optional 2D/3D physics, AI modules, audio mixing, a CPU-painted **retained GUI** toolkit, and optional **Dear ImGui** for immediate-mode tools.

## What You Get

| Layer | Technology | Public API |
|-------|------------|------------|
| Window / input | GLFW | `Window`, `IInput` |
| Rendering | Vulkan (internal) | `SceneRenderParams`, `IFramePresenter` |
| Simulation | ECS | `GameWorld`, `GameObject`, `GameComponent` |
| Physics | Custom solvers | `PhysicsSubsystem`, `PhysicsWorld2D`, `PhysicsWorld3D` |
| AI | FSM, GOAP, steering | `SimulateGameAi`, `AiAgentComponent` |
| Audio | Software mixer | `SoundEngine`, `SoundCueComponent` |
| UI (retained) | Widget tree → screen draws | `GuiCanvasComponent`, `spark/gui/` |
| UI (tools, optional) | Dear ImGui docking | `IImGuiLayer`, `GuiToolkitSettings` |

## Design Philosophy

1. **Explicit frame contract** — simulation in `OnUpdate`, rendering as immutable `SceneRenderParams` in `OnRender`.
2. **Dependency inversion** — games depend on `IGame` / `IEngineContext`, not `VulkanRenderer`.
3. **Composition** — behavior lives in components; `GameObject` is a lightweight container.
4. **Demos as documentation** — `src/spark/demo/` and `SparkDemo` showcase every subsystem.

## Minimal Bootstrap

```cpp
#include "spark/engine/Engine.hpp"
#include "spark/engine/Game.hpp"

class HelloSpark final : public Spark::Game {
public:
    void OnAttach(Spark::IEngineContext& ctx) override {
        auto* go = GetWorld().CreateGameObject();
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

## Default Demo Binary

`src/main.cpp` constructs `Engine` with `NewShellDemoGame()` — an interactive launcher with **19** built-in modes (3D fly scenes, maze, 2D games, scene editor prototype, material showcase, **Dear ImGui docking demo**, and more).

Next: [Engine Capabilities](02-engine-capabilities.md).
