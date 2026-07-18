#!/usr/bin/env python3
"""Generate comprehensive Spark Game Engine programming guide."""

from pathlib import Path

ROOT = Path(__file__).parent


def write(rel: str, content: str) -> None:
    p = ROOT / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(content.strip() + "\n", encoding="utf-8")


def chapter(folder: str, fname: str, title: str, order: int, body: str) -> None:
    fm = f"---\ntitle: {title}\norder: {order}\n---"
    write(f"{folder}/{fname}", fm + "\n" + body)


write("index.md", """---
title: Spark Game Engine Programming Guide
order: 0
---

# Spark Game Engine — Programming Guide

A comprehensive developer guide for building **2D and 3D games** with Spark — a **C++23** engine using GLFW, Vulkan, ECS-style entities, custom physics, AI modules, and retained-mode GUI.

## Who This Guide Is For

- C++ gameplay programmers and engine contributors
- Teams evaluating Spark for desktop games
- Developers migrating from Unity/Unreal to a code-first workflow

## Prerequisites

- C++23, CMake 3.28+, Vulkan SDK
- Vectors, matrices, basic rendering concepts
- Spark repository cloned locally

## Eight Parts (45 Chapters)

| Part | Folder | Focus |
|------|--------|-------|
| **1** | `1-overview-architecture/` | Engine loop, interfaces, ECS, render contract |
| **2** | `2-2d-graphics/` | Sprites, cameras, tilemaps, 2D pipeline |
| **3** | `3-3d-graphics/` | Meshes, PBR, lighting, skinning, terrain |
| **4** | `4-ai/` | Blackboard, FSM, GOAP, pathfinding, steering |
| **5** | `5-physics/` | 2D/3D solvers, colliders, queries, layers |
| **6** | `6-sound/` | Mixer, clips, cues, background music |
| **7** | `7-2d-game/` | Full platformer walkthrough |
| **8** | `8-3d-game/` | Full FPS arena walkthrough |

## Repository Map

| Path | Role |
|------|------|
| `include/spark/` | Public API |
| `src/spark/` | Implementations |
| `samples/platformer2d_game_template/` | 2D game sample |
| `samples/fps_game_template/` | 3D FPS sample |
| `docs/ARCHITECTURE_AND_DEVELOPER_GUIDE.md` | Contributor deep-dive |

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
./build/SparkDemo
```

Start with [Introduction](overview-architecture/01-introduction.html).
""")

# ─────────────────────────────────────────────────────────────────────────────
# PART 1 — Overview & Architecture
# ─────────────────────────────────────────────────────────────────────────────
P1 = "1-overview-architecture"

chapter(P1, "01-introduction.md", "Introduction to Spark", 1, """
# Introduction to Spark

Spark is a desktop-focused **C++23** game engine. It combines a GLFW window, Vulkan forward renderer, entity-component scene graph, optional 2D/3D physics, AI modules, audio mixing, and a CPU-painted GUI toolkit.

## What You Get

| Layer | Technology | Public API |
|-------|------------|------------|
| Window / input | GLFW | `Window`, `IInput` |
| Rendering | Vulkan (internal) | `SceneRenderParams`, `IFramePresenter` |
| Simulation | ECS | `GameWorld`, `GameObject`, `GameComponent` |
| Physics | Custom solvers | `SimulatePhysics2D`, `SimulatePhysics3D` |
| AI | FSM, GOAP, steering | `SimulateGameAi`, `AiAgentComponent` |
| Audio | Software mixer | `SoundEngine`, `SoundCueComponent` |
| UI | Retained widgets | `GuiCanvasComponent` |

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

`src/main.cpp` constructs `Engine` with `NewShellDemoGame()` — an interactive launcher (3D fly scenes, maze, 2D platformer, GUI gallery, material showcase).

Next: [Engine Capabilities](overview-architecture/02-engine-capabilities.html).
""")

chapter(P1, "02-engine-capabilities.md", "Engine Capabilities", 2, """
# Engine Capabilities

## Rendering Pipeline (High Level)

```mermaid
flowchart LR
    ECS[GameWorld ECS] --> Fill[FillStandardLitSceneFromWorld]
    Fill --> SRP[SceneRenderParams]
    GUI[PaintGuiCanvases] --> SRP
    SRP --> VK[VulkanRenderer]
    VK --> Present[Swapchain Present]
```

## 3D Rendering

| Feature | Types / Components |
|---------|-------------------|
| Forward PBR | `MaterialComponent`, `SceneShadingModel::LitPbr` |
| Toon / cel | `SceneShadingModel::ToonCel` |
| Directional + CSM | Sun vector in `SceneRenderParams` |
| Point / spot lights | `PointLightComponent`, `SpotLightComponent` (clustered) |
| Skinned characters | `SkinnedMeshComponent`, `AnimatorComponent` |
| Terrain | `TerrainComponent` (heightfield) |
| Sky | `SkyComponent` + `SceneSkyMode` |
| Particles | `ParticleEmitterComponent` |
| SSAO / IBL | Fields on `SceneRenderParams` |

## 2D Rendering

| Feature | Types |
|---------|-------|
| Sprites | `SpriteComponent` → `SceneSpriteDraw` |
| Tilemaps | `TilemapComponent` |
| Orthographic camera | `Camera2D` |
| Y-sort occlusion | `SceneSpriteSortMode::SortOrderThenWorldY` |
| 2D sprite lighting modes | `SpriteLighting2DMode` on draw items |

## Simulation & Tools

```cpp
Spark::SimulatePhysics2D(world, timing, settings);
Spark::SimulatePhysics3D(world, timing, settings);
Spark::SimulateGameAi(world, timing, context);
Spark::ProcessSoundCues(world, context.TryGetSoundEngine());
Spark::ProcessGuiCanvasesInput(scene, input, fbW, fbH);
```

## Asset Loading on GameWorld

```cpp
Spark::GltfAsset asset = world.LoadGltf("assets/models/Cube.glb");
Spark::SkinnedGltfAsset fox = world.LoadSkinnedGltf("assets/models/Fox.glb");
auto tex = world.LoadTexture("assets/sprites/player.png");
world.RegisterMesh(mesh, "my_game/hero");
world.RegisterTexture(tex, "my_game/hero_albedo");
```

Next: [Building and Running](overview-architecture/03-building-and-running.html).
""")

chapter(P1, "03-building-and-running.md", "Building and Running", 3, """
# Building and Running

## Requirements

| Tool | Version |
|------|---------|
| CMake | ≥ 3.28 |
| C++ | C++23 |
| Vulkan SDK | For shader compilation at build time |
| Network | First configure downloads GLFW, fonts, glTF samples |

## Configure & Build

```bash
cd /path/to/spark
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/SparkDemo
```

## CMake Options (selected)

| Option | Default | Effect |
|--------|---------|--------|
| `SPARK_BUILD_DEMO` | ON | Builds `SparkDemo` launcher |
| `SPARK_BUILD_SCRIPT_HOST` | OFF | CoreCLR C# scripting host |
| `SPARK_BUILD_TESTS` | OFF | Unit tests |

## Generated Config

`build/include/spark/config.hpp` defines:

- `SPARK_BUILD_ASSETS_DIR` — runtime asset root
- `SPARK_UI_FONT_PATH` — bundled Roboto paths

```cpp
#include "spark/config.hpp"
Utf8String path(SPARK_BUILD_ASSETS_DIR);
path.AppendUtf8("/fonts/Roboto-Regular.ttf");
```

## Your Own Target

```cmake
add_executable(MyGame src/main.cpp src/MyGame.cpp)
target_link_libraries(MyGame PRIVATE SparkEngine)
target_compile_features(MyGame PRIVATE cxx_std_23)
```

Copy `game_template/` or `samples/platformer2d_game_template/` as a starting point.

Next: [The Engine Loop](overview-architecture/04-engine-loop.html).
""")

chapter(P1, "04-engine-loop.md", "The Engine Loop", 4, """
# The Engine Loop

## Class Design: `Engine`

`Spark::Engine` (`spark/engine/Engine.hpp`) owns the application lifetime:

| Responsibility | Detail |
|----------------|--------|
| Owns `IGame` | Unique pointer; destroyed on shutdown |
| Owns `VulkanRenderer` | Implements `IFramePresenter` |
| Owns `SoundEngine` | Pumped each frame |
| Drives loop | Poll → Update → Audio → Render → Present |

```cpp
class Engine {
public:
    explicit Engine(UniquePtr<IGame> game);
    void Run();

    template<typename GameType, typename... Args>
    static UniquePtr<IGame> NewGame(Args&&... args) {
        return UniquePtr<IGame>(new GameType(Forward<Args>(args)...));
    }
};
```

## Frame Sequence

```mermaid
sequenceDiagram
    participant E as Engine
    participant G as IGame
    participant S as SoundEngine
    participant V as VulkanRenderer
    E->>E: PollEvents + BeginInputFrame
    E->>G: OnUpdate(timing, context)
    E->>S: PumpFrame(delta)
    E->>G: OnRender(frame, context)
    E->>V: PresentFrame
```

## FrameTiming

```cpp
struct FrameTiming {
    float deltaTimeSeconds;   // Use for all motion
    float totalTimeSeconds;   // Elapsed since start
    std::uint64_t frameIndex;
};
```

## IEngineContext Facade

Games receive `IEngineContext` — a stable surface hiding engine internals:

```cpp
Window& GetWindow();
IInput& GetInput();
void GetFramebufferSize(int& outWidth, int& outHeight) const;
void SetSceneRenderParams(const SceneRenderParams& params);
SoundEngine* TryGetSoundEngine() noexcept;
Scene* TryGetScene() noexcept;
```

## Input Example

```cpp
void OnUpdate(const FrameTiming& timing, IEngineContext& context) override {
    IInput& in = context.GetInput();
    if (in.IsKeyPressedThisFrame(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(context.GetWindow().Handle(), GLFW_TRUE);
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F12))
        ; // Engine handles screenshot internally when bound
}
```

## Screenshots

F12 captures the framebuffer to `spark_runtime_assets/screenshots/`.

Next: [IGame and Game](overview-architecture/05-igame-contract.html).
""")

chapter(P1, "05-igame-contract.md", "IGame and Game", 5, """
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
""")

chapter(P1, "06-ecs-and-scene.md", "ECS and Scene", 6, """
# ECS and Scene

## Class Design: `GameObject`

`GameObject` is Spark's **entity**. It does not use inheritance for gameplay — behavior is composed from components.

| Field / API | Purpose |
|-------------|---------|
| `GetId()` | Stable `uint64_t` identifier |
| `GetName()` | `Utf8String` debug label |
| `GetParent()` / `SetParent()` | Scene hierarchy |
| `GetWorldMatrix()` | Composed TRS from root |
| `AddComponent<T>()` | Type-safe component factory |
| `GetComponent<T>()` | Lookup by `T::TypeKind` |
| `EmitSignal()` | Sibling messaging on same entity |

```cpp
template<typename T, typename... Args>
T* AddComponent(Args&&... args) {
    static_assert(std::is_base_of_v<GameComponent, T>);
    auto ptr = MakeUnique<T>(Forward<Args>(args)...);
    T* raw = ptr.Get();
    raw->InternalSetOwner(this);
    components.PushBack(UniquePtr<GameComponent>(ptr.Release()));
    raw->OnAttach(*this);
    return raw;
}
```

## Class Design: `GameComponent`

```cpp
class GameComponent {
public:
    virtual ComponentKind Kind() const noexcept = 0;
    virtual void OnAttach(GameObject& owner);
    virtual void OnDetach(GameObject& owner);
    virtual void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context);
    virtual void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload);
    virtual int UpdatePriority() const noexcept { return 0; }
};
```

Built-in priorities (`ComponentUpdatePriority`):

| Priority | Value | Examples |
|----------|-------|----------|
| `AnimationDriver` | 100 | Clip sampling |
| `AnimatorPlayback` | 200 | Skeleton pose |

## Class Design: `GameWorld`

`GameWorld` owns all `GameObject` instances and **asset caches**:

```cpp
GameObject* CreateGameObject();
void DestroyGameObject(GameObject* object);
bool SetParent(GameObject* child, GameObject* newParent);
void UpdateGameObjects(const FrameTiming& timing, IEngineContext& context);

// Asset cache
void RegisterMesh(const SharedPtr<Mesh>& mesh, const char* key);
SharedPtr<Mesh> FindMesh(const char* key) const;
GltfAsset LoadGltf(const char* path);
SkinnedGltfAsset LoadSkinnedGltf(const char* path);
void SetUiFont(const SharedPtr<Font>& font);
```

## Class Design: `Scene`

`Scene` is a **read-only query layer** over `GameWorld`:

```cpp
void ForEachDrawable(const DrawableFn& fn);
void ForEachSkinnedDrawable(const SkinnedFn& fn);
void ForEachSprite(const SpriteFn& fn);
void ForEachTilemap(const TilemapFn& fn);
void ForEachPointLight(const PointLightFn& fn);
void ForEachGuiCanvas(const GuiCanvasFn& fn);
void SetSpatialPartitionKind(ScenePartitionKind kind);  // UniformGrid or BVH
```

## Signals Example

```cpp
// TransformComponent emits SignalId::TransformChanged
void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override {
    if (id == SignalId::TransformChanged) {
        // Rebuild local bounds, etc.
    }
    if (id == SignalId::Physics2DTriggerOverlap) {
        GameObject* other = static_cast<GameObject*>(payload.ptr);
        (void)other;
    }
}
```

## Rendering Hand-off

```cpp
Spark::SceneRenderParams params{};
Spark::FillStandardLitSceneFromWorld(world, context, viewProj, camPos,
    lightDir, lightColor, lightIntensity, ambient, enableParticles,
    camRight, camUp, sceneTime, params);
Spark::PaintGuiCanvases(world, params, fbW, fbH);
context.SetSceneRenderParams(params);
```

Part 1 complete → **Part 2**: [Sprites](2d-graphics/01-sprites.html).
""")

# ─────────────────────────────────────────────────────────────────────────────
# PART 2 — 2D Graphics
# ─────────────────────────────────────────────────────────────────────────────
P2 = "2-2d-graphics"

chapter(P2, "01-sprites.md", "Sprites", 1, """
# Sprites

## Class Design: `SpriteComponent`

`SpriteComponent` (`spark/ecs/components/SpriteComponent.hpp`) attaches an **unlit alpha-blended quad** to an entity. The GPU draws a unit quad in XY at z=0; `TransformComponent` scales and positions it.

| Member / API | Role |
|--------------|------|
| `SetTexture(SharedPtr<Texture2D>)` | CPU RGBA atlas |
| `SetTint(Vector4)` | Per-sprite multiply |
| `SetUvRect(Vector4)` | Atlas bounds (minU, minV, maxU, maxV) |
| `SetSortOrder(int32_t)` | Draw order (higher = on top) |

Constructor signature:

```cpp
SpriteComponent(SharedPtr<Texture2D> inTexture, const Vector4& inTint,
                const Vector4& inUvRect, std::int32_t inSortOrder) noexcept;
```

## Class Design: `Texture2D`

CPU-side RGBA8 image uploaded by the renderer when referenced in `SceneRenderParams::sceneTextures` (max 16 layers per frame).

```cpp
class Texture2D {
public:
    std::uint32_t GetWidth() const noexcept;
    const Array<std::uint8_t>& GetRgba() const noexcept;
    void SetPixels(std::uint32_t w, std::uint32_t h, Array<std::uint8_t> bytes);

    static Texture2D CreateCheckerboard(std::uint32_t px, std::uint32_t cellPx,
                                          const Vector3& c0, const Vector3& c1);
    static Texture2D CreateSolid(std::uint32_t w, std::uint32_t h, const Vector4& rgba);
    static bool TryLoadFromFile(const char* path, Texture2D& out, bool flipVerticalOnLoad = true);
};
```

## Spawn a Sprite

```cpp
auto tex = MakeShared<Texture2D>(Utf8String("Hero"));
if (!Texture2D::TryLoadFromFile("assets/sprites/hero.png", *tex)) {
    *tex = Texture2D::CreateSolid(64, 64, Vector4{1, 0, 0, 1});
}
world.RegisterTexture(tex, "game/hero");

auto* go = world.CreateGameObject();
auto* tr = go->AddComponent<TransformComponent>();
tr->SetTranslation({2.0F, 1.0F, 0.04F});
tr->SetScale({1.2F, 1.2F, 1.0F});

auto* sprite = go->AddComponent<SpriteComponent>(
    tex,
    Vector4{1.0F, 1.0F, 1.0F, 1.0F},   // tint
    Vector4{0.0F, 0.0F, 1.0F, 1.0F}, // full atlas UV
    100);                             // sortOrder
```

## Atlas UV Animation

```cpp
void SetFrame(int col, int row, int cols, int rows) {
    const float du = 1.0F / static_cast<float>(cols);
    const float dv = 1.0F / static_cast<float>(rows);
    sprite->SetUvRect({col * du, row * dv, (col + 1) * du, (row + 1) * dv});
}
```

## Manual Draw Item (Advanced)

When not using `FillStandardLitSceneFromWorld`, append to `SceneRenderParams`:

```cpp
SceneSpriteDraw draw{};
draw.model = transform->GetWorldMatrix();
draw.tint = sprite->GetTint();
draw.uvRect = sprite->GetUvRect();
draw.sortOrder = sprite->GetSortOrder();
draw.textureLayer = world.ResolveTextureLayer(sprite->GetTexture());
params.sprites.PushBack(draw);
```

## Coordinate System

World space is **+Y up**. Framebuffer Y increases downward after `OrthographicVulkan` projection — `Camera2D` handles this conversion.

Next: [Camera2D](2d-graphics/02-camera2d.html).
""")

chapter(P2, "02-camera2d.md", "Camera2D", 2, """
# Camera2D

## Class Design: `Camera2D`

`Camera2D` (`spark/scene/Camera2D.hpp`) is a **plain struct** (not a component) holding orthographic view parameters:

| Field | Default | Meaning |
|-------|---------|---------|
| `position` | `(0,0,0)` | Camera center in world XY |
| `rotationRad` | `0` | Roll around Z |
| `halfExtentY` | `5` | Half-height of ortho frustum |
| `clipNearZ` / `clipFarZ` | `-500` / `500` | Depth range |

```cpp
struct Camera2D {
    Vector3 position{0.0F, 0.0F, 0.0F};
    float rotationRad = 0.0F;
    float halfExtentY = 5.0F;

    Matrix4 ViewMatrix() const noexcept;
    Matrix4 ViewProjection(float framebufferWidth, float framebufferHeight) const noexcept;
    void BillboardBasisWorld(Vector3& outRight, Vector3& outUp) const noexcept;
};
```

## Apply to Render Submit

```cpp
int fbW = 0, fbH = 0;
context.GetFramebufferSize(fbW, fbH);
const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));

Vector3 particleRight{}, particleUp{};
camera.BillboardBasisWorld(particleRight, particleUp);

SubmitStandardLitSceneFromWorld(
    GetWorld(), context, viewProj, camera.position,
    Vector3{0.3F, 0.86F, 0.36F}.Normalized(),  // sun direction
    Vector3{1.0F, 0.98F, 0.95F}, 0.85F,       // sun color + intensity
    Vector3{0.16F, 0.18F, 0.24F},             // ambient
    false,                                     // enableParticles
    particleRight, particleUp,
    sceneTimeSeconds,
    SceneSpriteSortMode::SortOrderThenWorldY);
```

## Smooth Follow (from Platformer Sample)

```cpp
const Vector3 p = playerTr->GetLocalTransform().translation;
const float follow = std::min(1.0F, 8.0F * timing.deltaTimeSeconds);
camera.position.x += (p.x - camera.position.x) * follow;
camera.position.y += ((p.y + 0.85F) - camera.position.y) * follow;
```

## Pixel-Perfect Tips

- Use integer world positions for tile-aligned art.
- Set `halfExtentY` so one world unit ≈ N screen pixels at your target resolution.
- Separate **sortOrder layers**: background `10`, gameplay `100`, VFX `200`, HUD via `TextOverlayComponent` or GUI.

Next: [Tilemaps](2d-graphics/03-tilemaps.html).
""")

chapter(P2, "03-tilemaps.md", "Tilemaps", 3, """
# Tilemaps

## Class Design: `TilemapComponent`

Renders a dense 2D grid of tiles from a single atlas texture.

```cpp
class TilemapComponent final : public GameComponent {
public:
    static constexpr std::uint16_t kEmptyTile = 0xFFFF;

    TilemapComponent(SharedPtr<Texture2D> inAtlas,
                     std::uint32_t mapW, std::uint32_t mapH,
                     std::uint32_t atlasTilesU, std::uint32_t atlasTilesV,
                     float inTileWorldSize, std::int32_t inSortOrderBase) noexcept;

    void Resize(std::uint32_t mapW, std::uint32_t mapH);
    void SetTile(std::uint32_t x, std::uint32_t y, std::uint16_t tileId);
    std::uint16_t GetTile(std::uint32_t x, std::uint32_t y) const noexcept;
};
```

| Parameter | Meaning |
|-----------|---------|
| `atlasTilesU/V` | Columns/rows in atlas |
| `tileWorldSize` | World units per tile edge |
| `sortOrderBase` | Base draw order for the map layer |
| `tileId` | Atlas cell index; `kEmptyTile` = hole |

## Build a Level Grid

```cpp
constexpr std::uint32_t kMapW = 40;
constexpr std::uint32_t kMapH = 12;
constexpr std::uint32_t kAtlasCols = 8;
constexpr std::uint32_t kAtlasRows = 4;

auto* mapGo = world.CreateGameObject();
mapGo->AddComponent<TransformComponent>()->SetTranslation({0.0F, 0.0F, 0.0F});

auto* map = mapGo->AddComponent<TilemapComponent>(
    tileAtlas, kMapW, kMapH, kAtlasCols, kAtlasRows, 1.0F, 5);

for (std::uint32_t x = 0; x < kMapW; ++x) {
    map->SetTile(x, 0, 3);           // ground row
    map->SetTile(x, kMapH - 1, 1);   // ceiling
}
map->SetTile(10, 4, TilemapComponent::kEmptyTile);  // gap
```

## Collision Pairing

Tilemaps are **visual only**. Add static colliders separately:

```cpp
void AddSolidPlatform(GameWorld& w, float x0, float y0, float x1, float y1) {
    GameObject* go = w.CreateGameObject();
    TransformComponent* tr = go->AddComponent<TransformComponent>();
    tr->SetTranslation({(x0+x1)*0.5F, (y0+y1)*0.5F, 0.01F});
    tr->SetScale({std::fabs(x1-x0), std::fabs(y1-y0), 1.0F});
    go->AddComponent<SpriteComponent>(tex, tint, Vector4{0,0,1,1}, 10);
    go->AddComponent<BoxCollider2DComponent>();
    go->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
}
```

## Procedural Atlas (No External Assets)

```cpp
tileTex = MakeShared<Texture2D>(Utf8String("Tiles"));
*tileTex = Texture2D::CreateCheckerboard(256, 32,
    Vector3{0.38F, 0.34F, 0.30F}, Vector3{0.16F, 0.48F, 0.30F});
world.RegisterTexture(tileTex, "level/tiles");
```

Next: [2D Animation](2d-graphics/04-2d-animation.html).
""")

chapter(P2, "04-2d-animation.md", "2D Animation", 4, """
# 2D Animation

## Sprite Sheet Playback

Manual frame stepping on `SpriteComponent`:

```cpp
struct AnimFrame { Vector4 uv; float duration; };
Array<AnimFrame> runLoop;

void TickSpriteAnim(const FrameTiming& timing, SpriteComponent* sprite) {
    static float accum = 0.0F;
    static int frame = 0;
    accum += timing.deltaTimeSeconds;
    if (accum >= runLoop[frame].duration) {
        accum = 0.0F;
        frame = (frame + 1) % static_cast<int>(runLoop.GetSize());
        sprite->SetUvRect(runLoop[frame].uv);
    }
}
```

## `SpriteAnimatorComponent`

For data-driven clips, use `SpriteAnimatorComponent` (`spark/ecs/components/SpriteAnimatorComponent.hpp`) — pairs with `Sprite2DCharacterAnimFsmComponent` in character demos.

```cpp
auto* anim = go->AddComponent<SpriteAnimatorComponent>();
anim->SetClipSet(clipSet);
anim->Play(Spark::Utf8String("Run"));
```

Update priority ensures animation runs before render collection.

## Flip Without Extra Textures

```cpp
bool facingLeft = velocity.x < 0.0F;
tr->SetScale({facingLeft ? -baseScaleX : baseScaleX, baseScaleY, 1.0F});
```

## `TextOverlayComponent` HUD

Screen-space text without full GUI:

```cpp
auto* hud = world.CreateGameObject();
auto* text = hud->AddComponent<TextOverlayComponent>();
text->SetScreenPosition(12.0F, 12.0F);
text->SetFontSizePixels(20.0F);
text->SetColor({0.94F, 0.97F, 1.0F});
text->SetText(Utf8String("Score: 0"));
```

Requires `world.SetUiFont(font)` — see platformer `MountUiFontIfNeeded`.

## Squash and Stretch (Juice)

```cpp
const float vy = playerRb->GetVelocity().y;
const float stretch = 1.0F + std::clamp(vy * 0.02F, -0.15F, 0.15F);
playerTr->SetScale({baseScaleX / stretch, baseScaleY * stretch, 1.0F});
```

Next: [2D Lighting](2d-graphics/05-2d-lighting.html).
""")

chapter(P2, "05-2d-lighting.md", "2D Lighting", 5, """
# 2D Lighting

## `SpriteLighting2DMode`

Each `SceneSpriteDraw` can carry 2D lighting parameters (`spark/render/SpriteLighting2D.hpp`):

| Mode | Effect |
|------|--------|
| `None` | Flat tint only (default) |
| `NormalMapped` | Per-pixel normal lighting |
| `Ramp` | Gradient ramp lookup |

Fields on `SceneSpriteDraw`:

```cpp
SpriteLighting2DMode lightingMode = SpriteLighting2DMode::None;
Vector4 lightingParam0{1, 1, 1, 1};
Vector4 lightingParam1{1, 0, 0, 0};
```

## Directional Light on 2D Scenes

`SubmitStandardLitSceneFromWorld` accepts a **sun direction** even for sprite-only scenes — tints sprites when lighting mode is enabled:

```cpp
SubmitStandardLitSceneFromWorld(
    world, context, viewProj, camera.position,
    Vector3{0.30F, 0.86F, 0.36F}.Normalized(),  // lightDirectionWorld
    Vector3{1.0F, 0.98F, 0.95F},                 // lightColor
    0.85F,                                       // lightIntensity
    Vector3{0.16F, 0.18F, 0.24F},               // ambientColor
    false, pr, pu, sceneTime,
    SceneSpriteSortMode::SortOrderThenWorldY);
```

## Y-Sort for Top-Down ARPG

```cpp
params.spriteSortMode = SceneSpriteSortMode::SortOrderThenWorldY;
```

Within the same `sortOrder`, sprites with **lower world Y** draw on top (southern characters occlude northern).

## Fake Point Lights

Spawn additive-style sprites at lamp positions with high `sortOrder` and soft gradient textures.

See `docs/2D_ARPG_FEATURES.md` for ARPG-specific roadmap notes.

Next: [2D Render Pipeline](2d-graphics/06-2d-render-pipeline.html).
""")

chapter(P2, "06-2d-render-pipeline.md", "2D Render Pipeline", 6, """
# 2D Render Pipeline

## Class Design: `SceneRenderParams` (2D subset)

| Field | Limit | Purpose |
|-------|-------|---------|
| `sprites` | 8192 | `SceneSpriteDraw` array |
| `sceneTextures` | 16 | GPU texture table |
| `screenTexts` | — | HUD strings |
| `screenRects` | — | GUI / debug rects |
| `spriteSortMode` | enum | Sort policy |

```cpp
enum class SceneSpriteSortMode : std::uint8_t {
    SortOrderOnly = 0,
    SortOrderThenWorldY = 1,
};
```

## End-to-End 2D Frame (Platformer Pattern)

```cpp
void Platformer2DGame::OnRender(IRenderFrame&, IEngineContext& context) {
    int fbW = 0, fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) fbW = 1;
    if (fbH <= 0) fbH = 1;

    const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
    Vector3 pr{}, pu{};
    camera.BillboardBasisWorld(pr, pu);

    SubmitStandardLitSceneFromWorld(
        GetWorld(), context, viewProj, camera.position,
        Vector3{0.30F, 0.86F, 0.36F}.Normalized(),
        Vector3{1.0F, 0.98F, 0.95F}, 0.85F,
        Vector3{0.16F, 0.18F, 0.24F},
        false, pr, pu, sceneTimeSeconds,
        SceneSpriteSortMode::SortOrderThenWorldY);
}
```

`FillStandardLitSceneFromWorld` walks ECS and fills sprites, tilemaps, text overlays, and optional particles.

## GUI Overlay on 2D

```cpp
SceneRenderParams params{};
FillStandardLitSceneFromWorld(..., params);
PaintGuiCanvases(GetWorld(), params, fbW, fbH);
context.SetSceneRenderParams(params);
```

## Update Order Reminder

```cpp
void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    // 1. Read input, set velocities
    // 2. SimulatePhysics2D(world, t, settings)  — explicit call!
    // 3. Gameplay rules (respawn, goals)
    Game::OnUpdate(t, ctx);  // component OnUpdate + sound cues
}
```

Part 2 complete → **Part 3**: [Meshes and Materials](3d-graphics/01-meshes-and-materials.html).
""")

print("Part 2 done")

# ─────────────────────────────────────────────────────────────────────────────
# PART 3 — 3D Graphics
# ─────────────────────────────────────────────────────────────────────────────
P3 = "3-3d-graphics"

chapter(P3, "01-meshes-and-materials.md", "Meshes and Materials", 1, """
# Meshes and Materials

## Class Design: `Mesh`

CPU vertex/index buffers. Loaders and primitives live on `Mesh` (`spark/scene/Mesh.hpp`):

```cpp
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
};

static Mesh CreateUnitCube();
static Mesh CreateGroundPlane(float halfExtent);
static Mesh CreateSkyDome(float radius, int latSegs, int lonSegs);
static bool TryLoadFromObj(const char* path, Mesh& outMesh);
static bool TryLoadFromGltf(const char* path, Mesh& outMesh, SharedPtr<Texture2D>* outBaseColor = nullptr);
```

## Class Design: `MeshComponent`

```cpp
MeshComponent(SharedPtr<Mesh> mesh, SceneMeshSlot slot, Vector3 albedo);
void SetMesh(SharedPtr<Mesh> m);
void SetMeshSlot(SceneMeshSlot slot);
```

| `SceneMeshSlot` | Use |
|-----------------|-----|
| `UnitCube` | Built-in GPU cube |
| `GroundPlane` | Built-in plane |
| `Custom` | Upload `Mesh` vertices each frame |

## Class Design: `MaterialComponent`

```cpp
class MaterialComponent final : public GameComponent {
public:
    MaterialComponent(SharedPtr<Texture2D> baseColor, Vector3 inTint = Vector3::One);
    void SetMetallic(float m);
    void SetRoughness(float r);
    void SetEmissive(const Vector3& rgb, float intensity);
    void SetShadingModel(SceneShadingModel s);  // LitPbr, ToonCel, ...
    SharedPtr<Texture2D> GetNormalTexture() const noexcept;
    SharedPtr<Texture2D> GetMetallicRoughnessTexture() const noexcept;
};
```

`ApplyMaterialComponentToSceneDrawItem()` copies PBR fields into `SceneDrawItem`.

## Spawn a Lit glTF Prop

```cpp
GltfAsset asset = world.LoadGltf("assets/models/Crate.glb");
auto* go = world.CreateGameObject();
go->AddComponent<TransformComponent>()->SetTranslation({0, 0, 0});
go->AddComponent<MeshComponent>(asset.mesh, SceneMeshSlot::Custom, Vector3::One);

auto* mat = go->AddComponent<MaterialComponent>(asset.baseColorTexture);
mat->SetRoughness(0.55F);
mat->SetMetallic(0.1F);
mat->SetShadingModel(SceneShadingModel::LitPbr);
```

## Procedural Cube (FPS Sample Style)

```cpp
auto unitCube = MakeShared<Mesh>(Mesh::CreateUnitCube());
world.RegisterMesh(unitCube, "fps/unit_cube");

auto* target = world.CreateGameObject();
target->AddComponent<TransformComponent>()->SetTranslation({5, 1, -3});
target->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::Custom, Vector3{0.9F, 0.3F, 0.2F});
target->AddComponent<MaterialComponent>(nullptr)->SetRoughness(0.35F);
```

Next: [Cameras in 3D](3d-graphics/02-cameras-3d.html).
""")

chapter(P3, "02-cameras-3d.md", "Cameras in 3D", 2, """
# Cameras in 3D

## Class Design: `FlyCamera`

FPS-style camera struct (`spark/scene/FlyCamera.hpp`):

```cpp
struct FlyCamera {
    Vector3 position{0.0F, 4.2F, 16.0F};
    float yaw = 0.0F;
    float pitch = 0.0F;
    float moveSpeed = 5.0F;
    float mouseSensitivity = 0.12F;

    void SnapLookAt(const Vector3& target) noexcept;
    void AddLook(float deltaX, float deltaY) noexcept;
    void ProcessMovement(IInput& input, float deltaSeconds) noexcept;
    Vector3 Forward() const noexcept;
    Matrix4 ViewMatrix() const noexcept;
};
```

## Typical FPS Update Loop

```cpp
void OnAttach(IEngineContext& context) override {
    context.GetInput().SetCursorCaptured(true);
}

void OnUpdate(const FrameTiming& timing, IEngineContext& context) override {
    IInput& in = context.GetInput();
    if (in.IsCursorCaptured()) {
        camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        camera.ProcessMovement(in, timing.deltaTimeSeconds);
    }
    Game::OnUpdate(timing, context);
}
```

## Perspective View-Projection

```cpp
int fbW = 1, fbH = 1;
context.GetFramebufferSize(fbW, fbH);
const float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(72.0F), aspect, 0.1F, 200.0F);
const Matrix4 viewProj = proj * camera.ViewMatrix();
```

## `CharacterCameraRig`

For third-person characters, see `spark/scene/CharacterCameraRig.hpp` and `CharacterCameraDemo` — orbit camera with collision pull-in.

Next: [Lighting](3d-graphics/03-lighting.html).
""")

chapter(P3, "03-lighting.md", "Lighting", 3, """
# Lighting

## ECS Light Components

| Component | GPU representation |
|-----------|-------------------|
| `PointLightComponent` | `ScenePointLight` (position, range, color, intensity) |
| `SpotLightComponent` | `SceneSpotLight` (cone angles, direction) |
| Sun (global) | Passed to `FillStandardLitSceneFromWorld` |

```cpp
auto* lamp = world.CreateGameObject();
lamp->AddComponent<TransformComponent>()->SetTranslation({3, 2, 0});
auto* pl = lamp->AddComponent<PointLightComponent>();
pl->SetColor({1.0F, 0.85F, 0.6F});
pl->SetIntensity(4.0F);
pl->SetRange(12.0F);
pl->SetCastsShadow(true);
```

## Submit with Lighting Arguments

```cpp
SubmitStandardLitSceneFromWorld(
    GetWorld(), context, viewProj, camera.position,
    Vector3{0.25F, -1.0F, 0.15F}.Normalized(),  // directional "sun"
    Vector3{1.0F, 0.97F, 0.92F},                 // sun color
    3.5F,                                        // sun intensity
    Vector3{0.12F, 0.14F, 0.20F},               // ambient hemisphere
    true,                                        // particles
    camRight, camUp, sceneTime);
```

## Clustered Forward

`VulkanRenderer` packs up to **256 point** and **128 spot** lights per frame via clustered shading (`VulkanClusteredForwardLights`).

## Scene Lighting Profile

`SceneLightingProfile` on `SceneRenderParams` controls exposure, tonemap, and IBL contribution — see `spark/render/SceneLightingProfile.hpp`.

Read engine docs: `docs/LIGHTING_AND_SHADOWS.md`, `docs/MATERIALS_AND_LIGHTING.md`.

Next: [Shadows](3d-graphics/04-shadows.html).
""")

chapter(P3, "04-shadows.md", "Shadows", 4, """
# Shadows

## Directional CSM

Cascaded shadow maps for the sun direction — configured on `SceneRenderParams`:

```cpp
params.enableDirectionalShadows = true;
params.shadowCascadeCount = 4;
params.shadowMapResolution = 2048;
params.shadowBias = 0.002F;
params.shadowNormalBias = 0.02F;
```

When using `SubmitStandardLitSceneFromWorld`, enable shadows via the directional light setup and ensure meshes cast shadows (default for opaque draws).

## Punctual Light Shadows

```cpp
pl->SetCastsShadow(true);   // PointLightComponent
sl->SetCastsShadow(true);   // SpotLightComponent
```

GPU cost scales with shadow-casting light count — budget carefully.

## Toon Shading + Shadows

```cpp
mat->SetShadingModel(SceneShadingModel::ToonCel);
mat->SetToonSteps(3);
mat->SetToonSmoothness(0.1F);
```

Toon materials still receive shadow terms — see `ToonShadingDemo`.

## Debugging Shadow Artifacts

| Artifact | Tweak |
|----------|-------|
| Peter-panning | Increase `shadowBias` |
| Shadow acne | Increase `shadowNormalBias` |
| Shimmering CSM | Adjust cascade splits in renderer settings |

Next: [Skinned Characters](3d-graphics/05-skinned-characters.html).
""")

chapter(P3, "05-skinned-characters.md", "Skinned Characters", 5, """
# Skinned Characters

## Class Design: `SkinnedGltfAsset`

```cpp
struct SkinnedGltfAsset {
    SharedPtr<SkinnedMesh> mesh;
    SharedPtr<Skeleton> skeleton;
    SharedPtr<Texture2D> baseColorTexture;
    std::uint32_t walkClipIndex = 0;
    Quaternion bindUpAlignment;
    float bindFacingYawOffset = 0.0F;
};
```

Load via `world.LoadSkinnedGltf("assets/models/Fox.glb")`.

## Components

```cpp
SkinnedGltfAsset fox = world.LoadSkinnedGltf("assets/models/Fox.glb");

auto* go = world.CreateGameObject();
go->AddComponent<TransformComponent>()->SetTranslation({0, 0, 0});

auto* skin = go->AddComponent<SkinnedMeshComponent>(fox.mesh);
auto* mat = go->AddComponent<MaterialComponent>(fox.baseColorTexture);
mat->SetRoughness(0.6F);

auto* anim = go->AddComponent<AnimatorComponent>(fox.skeleton);
anim->PlayClip(fox.walkClipIndex, AnimLoopMode::Loop);
```

## Class Design: `AnimatorComponent`

- Samples animation clips into joint palette
- `UpdatePriority` = `AnimatorPlayback` (runs after `AnimationDriver`)
- Emits skinned draws with `jointPalette` in `SceneDrawItem`

## Attachment Points

Parent a weapon `GameObject` to the character root; offset in local space. For bone-accurate sockets, query skeleton pose after animation update (see character demos).

Next: [Terrain and Sky](3d-graphics/06-terrain-and-sky.html).
""")

chapter(P3, "06-terrain-and-sky.md", "Terrain and Sky", 6, """
# Terrain and Sky

## Class Design: `TerrainComponent`

Heightfield mesh with brush editing API:

```cpp
explicit TerrainComponent(TerrainGeneratorSettings settings, Vector3 meshAlbedo = ...);

void ResetHeightsToProcedural(GameObject& owner);
void RegenerateMesh(GameObject& owner);
bool TryRaycastWorld(const GameObject& owner, Vector3 rayOriginWorld,
                     Vector3 rayDirWorld, float maxDistance, Vector3& outHitWorld) const;
void ApplyHeightBrushWorld(GameObject& owner, Vector3 centerWorld,
                           float radiusWorld, float deltaY);
```

```cpp
TerrainGeneratorSettings settings{};
settings.gridCellsX = 128;
settings.gridCellsZ = 128;
settings.cellWorldSize = 1.0F;

auto* terrainGo = world.CreateGameObject();
terrainGo->AddComponent<TransformComponent>();
terrainGo->AddComponent<TerrainComponent>(settings);
```

## Class Design: `SkyComponent`

```cpp
explicit SkyComponent(SceneSkyMode mode) noexcept;  // Box, Dome, Plane
void SetSkyTexture(SharedPtr<Texture2D> t);
void SetTint(const Vector3& c) noexcept;
```

Pair with `MeshComponent` using matching sky mesh (`CreateSkyDome`) and `SceneSkyMode` on the draw item.

## Fog

```cpp
params.fogEnabled = true;
params.fogColor = {0.65F, 0.75F, 0.85F};
params.fogDensity = 0.015F;
```

See `TimeOfDayDemo` for sun/sky/fog animation.

Next: [Particles](3d-graphics/07-particles.html).
""")

chapter(P3, "07-particles.md", "Particles", 7, """
# Particles

## Class Design: `ParticleEmitterComponent`

CPU-simulated billboard particles collected into `SceneRenderParams::particles`:

```cpp
class ParticleEmitterComponent final : public GameComponent {
public:
    void SetEmissionRate(float rate) noexcept;
    void SetLifetime(float minSec, float maxSec) noexcept;
    void SetStartEndSize(float start, float end) noexcept;
    void SetGravity(const Vector3& g) noexcept;
    void SetEmissionDirection(const Vector3& dir) noexcept;
    void CollectInstances(Array<SceneParticleInstance>& out) const;
};
```

Requires `TransformComponent` on the same entity (emission origin = world translation).

## Fire Emitter Example

```cpp
auto* fxGo = world.CreateGameObject();
fxGo->AddComponent<TransformComponent>()->SetTranslation({0, 1, 0});

auto* emitter = fxGo->AddComponent<ParticleEmitterComponent>();
emitter->SetEmissionRate(80.0F);
emitter->SetLifetime(0.3F, 0.9F);
emitter->SetStartEndSize(0.15F, 0.02F);
emitter->SetGravity({0, 2.0F, 0});
emitter->SetEmissionDirection({0, 1, 0});
```

Enable particle collection in submit:

```cpp
SubmitStandardLitSceneFromWorld(..., enableParticles = true, camRight, camUp, sceneTime);
```

## `SceneParticleInstance`

```cpp
struct SceneParticleInstance {
    Vector3 position{};
    float size = 0.1F;
    Vector4 color{1, 1, 1, 1};
};
```

See `ParticleDemo` for colored bursts and muzzle flash patterns.

Part 3 complete → **Part 4**: [AI Overview](4-ai/01-ai-overview.html).
""")

print("Part 3 done")

# ─────────────────────────────────────────────────────────────────────────────
# PART 4 — AI
# ─────────────────────────────────────────────────────────────────────────────
P4 = "4-ai"

chapter(P4, "01-ai-overview.md", "AI Overview", 1, """
# AI Overview

## Architecture

```mermaid
flowchart TB
    Update[OnUpdate gameplay] --> Sim[SimulateGameAi]
    Sim --> Agent[AiAgentComponent::SubsystemTick]
    Agent --> FSM[FsmStateMachine]
    Agent --> GOAP[GoapPlanner]
    Agent --> Path[GridPathfinder polyline]
    Agent --> Steer[SteeringComposer]
    Agent --> BB[AiBlackboard]
```

## Entry Point

```cpp
#include "spark/ai/SimulateGameAi.hpp"

void OnUpdate(const FrameTiming& timing, IEngineContext& context) override {
    // Gameplay input first
    Game::OnUpdate(timing, context);
    SimulateGameAi(GetWorld(), timing, context);
}
```

`SimulateGameAi` iterates all `AiAgentComponent` instances where `IsEnabled()`.

## Class Design: `AiAgentComponent`

Central ECS hook (`spark/ecs/components/AiAgentComponent.hpp`):

| Module | Enable flag | Storage |
|--------|-------------|---------|
| Blackboard | always | `AiBlackboard` |
| FSM | `fsmEnabled` | `UniquePtr<FsmStateMachine>` |
| GOAP | `goapEnabled` | `goapActions`, `goapPlan` |
| Path follow | implicit | `pathWorldXZ`, `pathIndex` |
| Fuzzy | `fuzzyEnabled` | `UniquePtr<FuzzyAdvisoryModule>` |
| Motion | — | `maxSpeed`, `AiSteeringPlane` |

```cpp
auto* agent = enemy->AddComponent<AiAgentComponent>();
agent->SetMaxSpeed(6.0F);
agent->SetSteeringPlane(AiSteeringPlane::XzWorld);  // or XyRigidbody2D
agent->SetFsmEnabled(true);
agent->SetFsm(MakeUnique<FsmStateMachine>(/* ... */));
```

## Steering Planes

| `AiSteeringPlane` | Maps steering to |
|-------------------|------------------|
| `XzWorld` | World XZ (`Vector2.x` = X, `.y` = Z) |
| `XyRigidbody2D` | `Rigidbody2DComponent` velocity XY |

Next: [Steering Behaviors](4-ai/02-steering.html).
""")

chapter(P4, "02-steering.md", "Steering Behaviors", 2, """
# Steering Behaviors

## Class Design: `ISteeringBehavior` (2D)

```cpp
class ISteeringBehavior {
public:
    virtual Vector2 ComputeAcceleration(
        const Vector2& positionXZ,
        const Vector2& velocityXZ,
        AiBlackboard& board) const = 0;
};
```

Implementations in `spark/ai/steering/SteeringBehaviors.hpp`:

- `SteeringSeek`, `SteeringFlee`, `SteeringArrive`, `SteeringWander`
- `SteeringPursuit`, `SteeringEvade`, `SteeringSeparation`

## `SteeringComposer`

Weighted sum of behaviors:

```cpp
SteeringComposer composer;
composer.Add(&seek, 1.0F);
composer.Add(&separation, 0.8F);
Vector2 accel = composer.Compose(posXZ, velXZ, blackboard);
```

## 3D Steering

`spark/ai/steering/SteeringBehaviors3D.hpp` provides `ISteeringBehavior3D` and `SteeringComposer3D`:

- `SteeringSeek3D`, `SteeringFlee3D`, `SteeringObstacleAvoidance3D`
- `SteeringFlocking3D` (separation + alignment + cohesion)

## Blackboard Targets

Store goal position in blackboard slots (indices are game-defined):

```cpp
AiBlackboard& bb = agent->GetBlackboard();
bb.SetFloat(0, targetPos.x);  // slot 0 = target X
bb.SetFloat(1, targetPos.z);  // slot 1 = target Z
```

Behaviors read slots in `ComputeAcceleration`.

## Apply to Rigidbody

```cpp
Vector2 vel = rb->GetVelocity();
vel += accel * timing.deltaTimeSeconds;
vel = vel.ClampLength(agent->GetMaxSpeed());
rb->SetVelocity(vel);
```

Next: [Finite State Machines](4-ai/03-fsm.html).
""")

chapter(P4, "03-fsm.md", "Finite State Machines", 3, """
# Finite State Machines

## Class Design: `FsmStateMachine`

```cpp
struct FsmTransition {
    std::uint32_t fromState;
    std::uint32_t eventId;
    std::uint32_t toState;
};

class FsmStateMachine {
public:
    void AddState(UniquePtr<IFsmState> state);
    void AddTransition(const FsmTransition& rule);
    void SetInitialState(std::uint32_t stateId) noexcept;
    bool SendEvent(std::uint32_t eventId, AiBlackboard& board);
    void Tick(const FrameTiming& timing, AiBlackboard& board);
};
```

## Implement a State

```cpp
class PatrolState final : public IFsmState {
public:
  explicit PatrolState(std::uint32_t id) : stateId(id) {}
  std::uint32_t GetId() const noexcept override { return stateId; }

  void OnEnter(AiBlackboard& board) override { (void)board; }
  void OnExit(AiBlackboard& board) override { (void)board; }
  void Tick(const FrameTiming& timing, AiBlackboard& board) override {
    (void)timing; (void)board;
    // wander, play anim, etc.
  }
private:
  std::uint32_t stateId;
};
```

## Wire FSM to Agent

```cpp
enum : std::uint32_t { kIdle = 1, kChase = 2, kAttack = 3 };
enum : std::uint32_t { kEvtSeePlayer = 100, kEvtLostPlayer = 101 };

auto fsm = MakeUnique<FsmStateMachine>();
fsm->AddState(MakeUnique<IdleState>(kIdle));
fsm->AddState(MakeUnique<ChaseState>(kChase));
fsm->AddTransition({kIdle, kEvtSeePlayer, kChase});
fsm->AddTransition({kChase, kEvtLostPlayer, kIdle});
fsm->SetInitialState(kIdle);

agent->SetFsmEnabled(true);
agent->SetFsm(MoveTemp(fsm));
```

## Events from Gameplay

```cpp
if (distanceToPlayer < 12.0F)
    agent->TryGetFsm()->SendEvent(kEvtSeePlayer, agent->GetBlackboard());
```

`AiAgentComponent::SubsystemTick` calls `fsm->Tick` when `fsmEnabled`.

Next: [GOAP](4-ai/04-goap.html).
""")

chapter(P4, "04-goap.md", "GOAP", 4, """
# Goal-Oriented Action Planning

## Class Design: `GoapActionSpec`

Symbolic actions with bitmask preconditions and effects:

```cpp
struct GoapActionSpec {
    std::uint64_t preMask, preValue;       // required world bits
    std::uint64_t effectSetMask, effectClearMask;
    float cost = 1.0F;
    std::uint32_t nameId = 0;
};
```

World state is a `uint64_t` bitfield — semantics are **game-defined**.

## Planner API

```cpp
#include "spark/ai/goap/GoapPlanner.hpp"

std::uint64_t world = agent->GetGoapWorldBits();
std::uint64_t goalMask = (1ull << 3);  // bit 3 must be set
std::uint64_t goalValue = (1ull << 3);

Array<std::uint32_t> plan;
bool ok = GoapPlanner::Plan(world, goalMask, goalValue,
                            agent->GetGoapActions(), plan);
```

## Register Actions

```cpp
agent->SetGoapEnabled(true);

GoapActionSpec pickup{};
pickup.preMask  = (1ull << 0);  // must be "near item"
pickup.preValue = (1ull << 0);
pickup.effectSetMask = (1ull << 1);  // has item
pickup.cost = 2.0F;
agent->GetGoapActions().PushBack(pickup);

GoapActionSpec deliver{};
deliver.preMask = (1ull << 1);
deliver.preValue = (1ull << 1);
deliver.effectSetMask = (1ull << 3);  // quest complete
deliver.cost = 1.0F;
agent->GetGoapActions().PushBack(deliver);

agent->SetGoapGoal((1ull << 3), (1ull << 3));
```

GOAP plans **what** to do; combine with pathfinding/steering for **how** to move.

See `GoapDemo` in `SparkDemo`.

Next: [Pathfinding](4-ai/05-pathfinding.html).
""")

chapter(P4, "05-pathfinding.md", "Pathfinding", 5, """
# Pathfinding

## Class Design: `GridPathfinder`

```cpp
#include "spark/ai/path/GridPathfinder.hpp"

class IGridWalkability {
public:
    virtual bool IsWalkable(int cellX, int cellZ) const = 0;
};

class GridBitmapWalkability final : public IGridWalkability { /* ... */ };

struct Cell { int x = 0; int z = 0; };

static bool FindPath4(const IGridWalkability& grid, const Cell& start,
                      const Cell& goal, Array<Cell>& outCells);
static void CellsToWorldPolyline(const Array<Cell>& cells, const Vector2& gridOriginXZ,
                                 float cellSize, Array<Vector2>& outWorldXZ);
```

4-connected A* on a grid abstraction.

## Build Walkability from Tilemap

```cpp
GridBitmapWalkability walk;
walk.Resize(mapW, mapH);
for (uint32_t y = 0; y < mapH; ++y)
  for (uint32_t x = 0; x < mapW; ++x)
    walk.SetWalkable(x, y, map->GetTile(x, y) != TilemapComponent::kEmptyTile);
```

## Store Path on Agent

```cpp
Array<Cell> cells;
Cell start{playerCellX, playerCellZ};
Cell goal{targetCellX, targetCellZ};
if (GridPathfinder::FindPath4(walk, start, goal, cells)) {
    auto& poly = agent->GetPathWorldPolylineXZ();
    GridPathfinder::CellsToWorldPolyline(cells, Vector2{-20, -5}, 1.0F, poly);
    agent->SetPathIndex(0);
}
```

## Follow Polyline

Advance `pathIndex` when within `arriveRadius` of each waypoint; steer toward `poly[pathIndex]`.

## Fuzzy Logic (Optional)

`FuzzyAdvisoryModule` (`spark/ai/fuzzy/FuzzyLogic.hpp`) blends continuous inputs (health, distance) into action weights — enable via `agent->SetFuzzyEnabled(true)`.

Part 4 complete → **Part 5**: [Physics Overview](5-physics/01-physics-overview.html).
""")

print("Part 4 done")

# ─────────────────────────────────────────────────────────────────────────────
# PART 5 — Physics
# ─────────────────────────────────────────────────────────────────────────────
P5 = "5-physics"

chapter(P5, "01-physics-overview.md", "Physics Overview", 1, """
# Physics Overview

## Design: Explicit Simulation

Spark physics is **not** auto-ticked in `Game::OnUpdate`. Your game calls simulation explicitly:

```cpp
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"

PhysicsWorld2DSettings settings2d{};
settings2d.gravityY = -30.0F;
settings2d.maxFallSpeed = 42.0F;
SimulatePhysics2D(world, timing, settings2d);

PhysicsWorld3DSettings settings3d{};
settings3d.gravityY = -24.0F;
settings3d.substeps = 1;
settings3d.resolveIterations = 8;
SimulatePhysics3D(world, timing, settings3d);
```

## Settings Structs

**2D** (`PhysicsWorld2DSettings`):

| Field | Default | Meaning |
|-------|---------|---------|
| `gravityY` | `-32` | Vertical acceleration |
| `maxFallSpeed` | `46` | Terminal velocity clamp |
| `resolveDynamicVsDynamic` | `false` | Dynamic-dynamic resolution |

**3D** (`PhysicsWorld3DSettings`):

| Field | Default |
|-------|---------|
| `gravityY` | `-24` |
| `substeps` | `1` |
| `resolveIterations` | `8` |
| `sweptStaticCcdBinaryIterations` | `10` |

## Recommended Update Order

```cpp
void OnUpdate(const FrameTiming& t, IEngineContext& ctx) override {
    ReadInputAndSetForces();
    SimulatePhysics2D(GetWorld(), t, physSettings);
    PostPhysicsGameplay();
    Game::OnUpdate(t, ctx);
}
```

Next: [Rigidbody 2D](5-physics/02-rigidbody-2d.html).
""")

chapter(P5, "02-rigidbody-2d.md", "Rigidbody 2D", 2, """
# Rigidbody 2D

## Class Design: `Rigidbody2DComponent`

```cpp
enum class RigidbodyBodyType2D { Kinematic, Static, Dynamic };

Rigidbody2DComponent(RigidbodyBodyType2D bodyType = Dynamic, float gravityScaleIn = 1.0F);

Vector2& GetVelocity();
void SetVelocity(const Vector2& v);
bool IsGrounded() const noexcept;
float GetGravityScale() const noexcept;
```

| Body type | Behavior |
|-----------|----------|
| `Static` | Immovable collider (platforms) |
| `Dynamic` | Simulated velocity + gravity |
| `Kinematic` | Moved by transform, pushes dynamics |

## Player Controller (Platformer)

```cpp
Vector2 v = playerRb->GetVelocity();
float run = 0.0F;
if (in.IsKeyDown(GLFW_KEY_A)) run -= 1.0F;
if (in.IsKeyDown(GLFW_KEY_D)) run += 1.0F;
v.x = run * kRunSpeed;  // kRunSpeed = 9.0F in sample

if (playerRb->IsGrounded() && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE))
    v.y = kJumpSpeed;   // kJumpSpeed = 11.5F

playerRb->SetVelocity(v);

PhysicsWorld2DSettings phys{};
phys.gravityY = -30.0F;
phys.maxFallSpeed = 42.0F;
SimulatePhysics2D(GetWorld(), timing, phys);
```

## Static Platform

```cpp
go->AddComponent<BoxCollider2DComponent>();
go->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
```

Transform scale defines collider world size when using default `BoxCollider2DComponent` half-extents.

Next: [Colliders](5-physics/03-colliders.html).
""")

chapter(P5, "03-colliders.md", "Colliders", 3, """
# Colliders

## 2D Colliders

**BoxCollider2DComponent:**

```cpp
BoxCollider2DComponent(Vector2 localHalfExtents = {0.5F, 0.5F},
                       Vector2 localOffset = Vector2::Zero);
```

**CircleCollider2DComponent:**

```cpp
CircleCollider2DComponent(float localRadius = 0.5F,
                          Vector2 localOffset = Vector2::Zero);
```

If both exist on a dynamic body, **circle wins** for simulation.

## Layer Masks

```cpp
auto* box = go->AddComponent<BoxCollider2DComponent>();
box->SetCategoryBits(1u << 0);   // layer 0 = player
box->SetMaskBits(0xFFFF);        // collides with all by default
box->SetIsTrigger(true);         // overlap only, no resolution
```

Collision filter (`CollisionFilter2D::ShouldCollide`):

```
collide = (maskA & categoryB) && (maskB & categoryA)
```

## 3D Colliders

| Component | Shape |
|-----------|-------|
| `BoxCollider3DComponent` | Axis-aligned box (static) |
| `SphereCollider3DComponent` | Sphere (dynamic) |

```cpp
go->AddComponent<Rigidbody3DComponent>();
go->AddComponent<SphereCollider3DComponent>(0.5F);
```

Optional: `PhysicsMaterial3DComponent` (friction/restitution), `DistanceJoint3DComponent`.

## Trigger Signals

```cpp
void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override {
    if (id == SignalId::Physics2DTriggerOverlap) {
        GameObject* other = static_cast<GameObject*>(payload.ptr);
        // pickup, damage zone, goal flag
    }
}
```

Next: [Physics Queries](5-physics/04-queries.html).
""")

chapter(P5, "04-queries.md", "Physics Queries", 4, """
# Physics Queries

## Class Design: `PhysicsQueries2D`

```cpp
struct PhysicsRaycastHit2D {
    GameObject* gameObject = nullptr;
    float hitX = 0, hitY = 0;
    float normalX = 0, normalY = 1;
    float distance = 0;
};

struct PhysicsQueryFilter2D {
    std::uint16_t queryCategoryBits = 0xFFFF;
    std::uint16_t queryMaskBits = 0xFFFF;
    bool hitSolids = true;
    bool hitTriggers = true;
};
```

## Raycast World Convenience

```cpp
#include "spark/physics/PhysicsQueries2D.hpp"

PhysicsRaycastHit2D hit{};
if (PhysicsQueries2D::RaycastWorld2D(
        world, originX, originY, dirX, dirY, maxDist, filter, hit)) {
    GameObject* struck = hit.gameObject;
    (void)struck;
}
```

## Static Broadphase (Manual)

```cpp
StaticBroadPhase2D broad;
broad.Rebuild(world, 4.0F);  // cellWorldSize

PhysicsRaycastHit2D hit{};
PhysicsQueries2D::RaycastStatics2D(broad, ox, oy, dx, dy, maxDist, filter, hit);
```

## Overlap Queries

```cpp
Array<GameObject*> hits;
PhysicsQueries2D::QueryOverlapCircleDynamics2D(world, cx, cy, radius, filter, hits);
PhysicsQueries2D::QueryOverlapArcStatics2D(broad, cx, cy, radius,
    startAngleRad, sweepRad, filter, hits);
```

Arc queries support cone attacks and vision checks.

## Ground Check Pattern

```cpp
bool grounded = PhysicsQueries2D::RaycastWorld2D(
    world, footX, footY, 0.0F, -1.0F, 0.08F, filter, hit);
```

Or use `Rigidbody2DComponent::IsGrounded()` after simulation.

Next: [Physics 3D](5-physics/05-physics-3d.html).
""")

chapter(P5, "05-physics-3d.md", "Physics 3D", 5, """
# Physics 3D

## Class Design: `Rigidbody3DComponent`

```cpp
Vector3& GetVelocity();
void SetVelocity(const Vector3& v);
Vector3& GetAngularVelocity();
void AddImpulse(const Vector3& impulse);
```

Pair with `SphereCollider3DComponent` for dynamic props.

## Throw Demo Pattern

```cpp
auto* ball = world.CreateGameObject();
ball->AddComponent<TransformComponent>()->SetTranslation(spawnPos);
ball->AddComponent<SphereCollider3DComponent>(0.25F);
auto* rb = ball->AddComponent<Rigidbody3DComponent>();
rb->SetVelocity(camera.Forward() * 18.0F);

SimulatePhysics3D(world, timing, settings);
```

See `PhysicsBallThrow3DDemo`.

## Spatial Hash

`SpatialHashGrid3D` accelerates static queries — rebuilt during simulation internally.

## Kinematic FPS Controller

The FPS sample uses **FlyCamera** without rigidbody — common for shooters. Physics 3D is for props, debris, and puzzles.

Next: [Tips and Patterns](5-physics/06-tips.html).
""")

chapter(P5, "06-tips.md", "Joints and Tips", 6, """
# Tips and Patterns

## Fixed Timestep

```cpp
float accum = 0.0F;
constexpr float kFixedDt = 1.0F / 60.0F;
accum += timing.deltaTimeSeconds;
while (accum >= kFixedDt) {
    SimulatePhysics2D(world, FrameTiming{kFixedDt, ...}, settings);
    accum -= kFixedDt;
}
```

## Layer Design Example

| Layer bit | Category | Collides with |
|-----------|----------|---------------|
| 0 | Player | 1, 2 |
| 1 | Environment | all |
| 2 | Enemy | 0, 1 |
| 3 | Pickup (trigger) | 0 |

## Dynamic vs Dynamic

Default `resolveDynamicVsDynamic = false` in 2D — enable only if you need pile-ups.

## Debug Visualization

Use `TextOverlayComponent` or GUI labels to show velocity:

```cpp
std::format("v=({:.1f},{:.1f})", v.x, v.y);
```

## When to Skip Physics

Grid-based tactics, visual novels, and menu scenes need no solver — omit `SimulatePhysics*`.

Part 5 complete → **Part 6**: [Sound Engine](6-sound/01-sound-engine.html).
""")

print("Part 5 done")

# ─────────────────────────────────────────────────────────────────────────────
# PART 6 — Sound
# ─────────────────────────────────────────────────────────────────────────────
P6 = "6-sound"

chapter(P6, "01-sound-engine.md", "Sound Engine", 1, """
# Sound Engine

## Class Design

```mermaid
flowchart LR
    Cue[SoundCueComponent::Queue] --> Process[ProcessSoundCues]
    Process --> Mixer[SoundMixer]
    Mixer --> Out[Platform Audio Output]
    BGM[SetBackgroundMusic] --> Mixer
    Engine[Engine::PumpFrame] --> Mixer
```

| Class | Role |
|-------|------|
| `SoundEngine` | Facade: startup, pump, BGM |
| `SoundMixer` | Voice pool, mixing |
| `SoundClip` | Decoded stereo float PCM |
| `SoundCueComponent` | Per-entity one-shot queue |

## SoundEngine API

```cpp
class SoundEngine {
public:
    bool Startup();
    void Shutdown() noexcept;
    void PumpFrame(float deltaTimeSeconds) noexcept;
    SoundMixer& GetMixer() noexcept;
    void SetBackgroundMusic(const SharedPtr<SoundClip>& clip,
                            float volume = 0.32F, bool loop = true) noexcept;
    void ClearBackgroundMusic() noexcept;
};
```

`Engine::Run` calls `PumpFrame` after `OnUpdate`. Games access audio via:

```cpp
SoundEngine* audio = context.TryGetSoundEngine();
if (audio) {
    audio->SetBackgroundMusic(ambienceClip, 0.25F, true);
}
```

Next: [Sound Clips](6-sound/02-clips.html).
""")

chapter(P6, "02-clips.md", "Sound Clips", 2, """
# Sound Clips

## Class Design: `SoundClip`

Decoded **interleaved stereo float** samples:

```cpp
class SoundClip {
public:
    const Array<float>& GetInterleavedStereo() const noexcept;
    std::uint32_t GetSampleRate() const noexcept;
    std::size_t GetFrameCount() const noexcept;

    static SharedPtr<SoundClip> CreateToneBlip(float frequencyHz,
                                               float durationSeconds, float gain);
    static SharedPtr<SoundClip> CreateSimpleAmbienceLoop();
};
```

Load from file via `GameWorld` or decode helpers in `spark/audio/`.

## Procedural SFX (No Assets)

```cpp
auto jumpSfx = SoundClip::CreateToneBlip(440.0F, 0.08F, 0.5F);
auto landSfx = SoundClip::CreateToneBlip(220.0F, 0.06F, 0.4F);
```

## Load WAV at Runtime

```cpp
// Pattern: load bytes, decode to SoundClip — check SoundClip loaders in spark/audio/
SharedPtr<SoundClip> clip = /* load from assets/audio/jump.wav */;
```

Supported formats depend on build configuration — WAV is always safe.

Next: [Sound Cues](6-sound/03-cues.html).
""")

chapter(P6, "03-cues.md", "Sound Cues", 3, """
# Sound Cues

## Class Design: `SoundCueComponent`

```cpp
class SoundCueComponent final : public GameComponent {
public:
    void Queue(const SharedPtr<SoundClip>& clip, float volume = 1.0F);
    void FlushTo(SoundEngine* engine) noexcept;
};
```

## Queue Pattern

```cpp
auto* player = playerObject;
auto* cue = player->GetComponent<SoundCueComponent>();
if (!cue) cue = player->AddComponent<SoundCueComponent>();

if (justJumped)
    cue->Queue(jumpClip, 0.9F);
if (justLanded)
    cue->Queue(landClip, 0.6F);
```

## Automatic Drain

`Game::OnUpdate` calls `ProcessSoundCues(world, context.TryGetSoundEngine())` which invokes `FlushTo` on every `SoundCueComponent`.

If you bypass `Game::OnUpdate`, call `ProcessSoundCues` yourself.

## Background Music

```cpp
if (SoundEngine* se = context.TryGetSoundEngine()) {
    se->SetBackgroundMusic(bgmClip, 0.28F, true);
}
// Later:
se->ClearBackgroundMusic();
```

## Multiple Voices

`SoundMixer` multiplexes one-shots — rapid `Queue` calls overlap without cutting off prior sounds (within voice pool limits).

Part 6 complete → **Part 7**: [Platformer Introduction](7-2d-game/01-platformer-intro.html).
""")

print("Part 6 done")

# ─────────────────────────────────────────────────────────────────────────────
# PART 7 — Complete 2D Game
# ─────────────────────────────────────────────────────────────────────────────
P7 = "7-2d-game"

chapter(P7, "01-platformer-intro.md", "Platformer Introduction", 1, """
# Platformer Introduction

We walk through `samples/platformer2d_game_template/` — a complete side-scroller with:

- Procedural checker tile art (no external sprites required)
- `Camera2D` with smooth follow
- `Rigidbody2DComponent` player + static platforms
- HUD via `TextOverlayComponent`
- Goal reach detection

## Class Design: `Platformer2DGame`

```cpp
class Platformer2DGame final : public Game {
    Camera2D camera{};
    SharedPtr<Texture2D> tileTex{};
    Array<GameObject*> roots{};
    GameObject* playerObject = nullptr;
    TransformComponent* playerTr = nullptr;
    Rigidbody2DComponent* playerRb = nullptr;
    TextOverlayComponent* hudText = nullptr;
};
```

## Entry Point

```cpp
#include "spark/engine/Engine.hpp"
#include "Platformer2DGame.hpp"

int main() {
    Spark::Engine engine(Spark::Engine::NewGame<Spark::Platformer2DGame>());
    engine.Run();
    return 0;
}
```

## Constants (Sample)

```cpp
constexpr float kRunSpeed = 9.0F;
constexpr float kJumpSpeed = 11.5F;
constexpr float kGroundTopY = -1.0F;
constexpr float kGoalMinX = 15.5F;
```

Next: [Project Setup](7-2d-game/02-project-setup.html).
""")

chapter(P7, "02-project-setup.md", "Project Setup", 2, """
# Project Setup

## Copy Template

```bash
cp -r samples/platformer2d_game_template my_platformer
```

## CMakeLists.txt Pattern

```cmake
add_executable(Platformer2D src/main.cpp src/Platformer2DGame.cpp)
target_link_libraries(Platformer2D PRIVATE SparkEngine)
target_compile_features(Platformer2D PRIVATE cxx_std_23)
```

## Font Mounting

UI text requires `GameWorld::SetUiFont`:

```cpp
void MountUiFontIfNeeded(GameWorld& world) {
    if (world.GetUiFont()) return;
    auto uiFont = MakeShared<Font>();
    if (uiFont->TryLoadTrueTypeFromFile(SPARK_UI_FONT_PATH, 42.0F))
        world.SetUiFont(uiFont);
}
```

Called from `OnAttach` before creating `TextOverlayComponent`.

## Track Roots for Cleanup

```cpp
void OnDetach() override {
    for (GameObject* go : roots)
        if (go) GetWorld().DestroyGameObject(go);
    roots.Clear();
}
```

Next: [Building the Level](7-2d-game/03-level-design.html).
""")

chapter(P7, "03-level-design.md", "Level Design", 3, """
# Building the Level

## `AddSolidPlatform` Helper

From the sample — one function spawns visual + collision:

```cpp
void AddSolidPlatform(GameWorld& w, Array<GameObject*>& roots,
        const SharedPtr<Texture2D>& tex,
        float x0, float y0, float x1, float y1,
        std::int32_t sortOrder, const Vector4& tint) {
    const float cx = (x0 + x1) * 0.5F;
    const float cy = (y0 + y1) * 0.5F;
    const float sx = std::fabs(x1 - x0);
    const float sy = std::fabs(y1 - y0);

    GameObject* go = w.CreateGameObject();
    TransformComponent* tr = go->AddComponent<TransformComponent>();
    tr->SetTranslation({cx, cy, 0.01F + 0.0004F * static_cast<float>(sortOrder)});
    tr->SetScale({sx, sy, 1.0F});
    go->AddComponent<SpriteComponent>(tex, tint, Vector4{0,0,1,1}, sortOrder);
    go->AddComponent<BoxCollider2DComponent>();
    go->AddComponent<Rigidbody2DComponent>(RigidbodyBodyType2D::Static, 0.0F);
    roots.PushBack(go);
}
```

## Level Layout

```cpp
tileTex = MakeShared<Texture2D>(Utf8String("Tiles"));
*tileTex = Texture2D::CreateCheckerboard(256, 32,
    Vector3{0.38F, 0.34F, 0.30F}, Vector3{0.16F, 0.48F, 0.30F});
world.RegisterTexture(tileTex, "platformer2d_template/tiles");

AddSolidPlatform(world, roots, tileTex, -12, -3, 24, kGroundTopY, 10, ...);
AddSolidPlatform(world, roots, tileTex, -2, -0.2, 2.5, 0.55, 20, ...);
AddSolidPlatform(world, roots, tileTex, 11, 2.4, 16.5, 3.05, 22, ...);
```

Z offsets separate draw order among platforms.

Next: [Player Controller](7-2d-game/04-player-controller.html).
""")

chapter(P7, "04-player-controller.md", "Player Controller", 4, """
# Player Controller

## Spawn Player

```cpp
playerObject = world.CreateGameObject();
playerTr = playerObject->AddComponent<TransformComponent>();
playerTr->SetScale({0.88F, 1.05F, 1.0F});
playerTr->SetTranslation({kPlayerSpawnX, spawnY, 0.04F});

playerObject->AddComponent<SpriteComponent>(
    tileTex, Vector4{0.35F, 0.55F, 0.95F, 1.0F},
    Vector4{0,0,1,1}, 500);

playerObject->AddComponent<BoxCollider2DComponent>();
playerRb = playerObject->AddComponent<Rigidbody2DComponent>(
    RigidbodyBodyType2D::Dynamic, 1.0F);
```

## Movement + Jump

```cpp
float run = 0.0F;
if (in.IsKeyDown(GLFW_KEY_A) || in.IsKeyDown(GLFW_KEY_LEFT)) run -= 1.0F;
if (in.IsKeyDown(GLFW_KEY_D) || in.IsKeyDown(GLFW_KEY_RIGHT)) run += 1.0F;

if (std::fabs(run) > 0.5F) facingLeft = (run < 0.0F);
playerTr->SetScale({facingLeft ? -baseScaleX : baseScaleX, baseScaleY, 1.0F});

Vector2 v = playerRb->GetVelocity();
v.x = run * kRunSpeed;
if (playerRb->IsGrounded() && in.IsKeyPressedThisFrame(GLFW_KEY_SPACE))
    v.y = kJumpSpeed;
playerRb->SetVelocity(v);

PhysicsWorld2DSettings phys{};
phys.gravityY = -30.0F;
SimulatePhysics2D(GetWorld(), timing, phys);
```

## Respawn + Goal

```cpp
if (p.y < kFallRespawnY) {
    playerTr->SetTranslation({kPlayerSpawnX, spawnY, p.z});
    playerRb->SetVelocity(Vector2::Zero);
}
if (!goalReached && p.x >= kGoalMinX)
    goalReached = true;
```

Next: [Camera and HUD](7-2d-game/05-camera-hud.html).
""")

chapter(P7, "05-camera-hud.md", "Camera and HUD", 5, """
# Camera and HUD

## Camera Follow

```cpp
camera.position = {kPlayerSpawnX, spawnY + 1.0F, 0.0F};
camera.halfExtentY = 6.5F;

// Each frame:
const float follow = std::min(1.0F, 8.0F * timing.deltaTimeSeconds);
camera.position.x += (p.x - camera.position.x) * follow;
camera.position.y += ((p.y + 0.85F) - camera.position.y) * follow;
```

## HUD Text

```cpp
hudText->SetText(Utf8String(std::format(
    "2D platformer {} | pos ({:.1f},{:.1f}) | A/D Space | ESC quit",
    goalReached ? "— GOAL!" : "",
    p.x, p.y).c_str()));
```

## Render Submit

```cpp
const Matrix4 viewProj = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
Vector3 pr{}, pu{};
camera.BillboardBasisWorld(pr, pu);

SubmitStandardLitSceneFromWorld(
    GetWorld(), context, viewProj, camera.position,
    Vector3{0.30F, 0.86F, 0.36F}.Normalized(),
    Vector3{1.0F, 0.98F, 0.95F}, 0.85F,
    Vector3{0.16F, 0.18F, 0.24F},
    false, pr, pu, sceneTimeSeconds,
    SceneSpriteSortMode::SortOrderThenWorldY);
```

Next: [Polish and Ship](7-2d-game/06-polish.html).
""")

chapter(P7, "06-polish.md", "Polish and Ship", 6, """
# Polish and Ship

## Sound

```cpp
auto* cue = playerObject->AddComponent<SoundCueComponent>();
if (justJumped) cue->Queue(SoundClip::CreateToneBlip(520, 0.07F, 0.5F));
```

## Pause Menu (GUI)

```cpp
#include "spark/gui/GuiControls.hpp"
#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/gui/GuiScene.hpp"

auto* uiGo = world.CreateGameObject();
auto* canvas = uiGo->AddComponent<GuiCanvasComponent>();
auto root = MakeUnique<Gui::StackPanel>();
auto btn = MakeUnique<Gui::Button>();
btn->SetLabel(Utf8String("Resume"));
btn->SetOnClick([] { paused = false; });
root->AddChild(MoveTemp(btn));
canvas->SetRoot(MoveTemp(root));
```

Frame flow:

```cpp
ProcessGuiCanvasesInput(GetScene(), input, fbW, fbH);
// ... fill params ...
PaintGuiCanvases(GetWorld(), params, fbW, fbH);
```

## Replace Procedural Art

Swap `CreateCheckerboard` for `Texture2D::TryLoadFromFile("assets/tiles.png", ...)`.

## Ship Checklist

- [ ] Release build (`CMAKE_BUILD_TYPE=Release`)
- [ ] Bundle `assets/` beside executable
- [ ] Test on target DPI / resolution
- [ ] Verify `SPARK_BUILD_ASSETS_DIR` paths

Part 7 complete → **Part 8**: [FPS Introduction](8-3d-game/01-fps-intro.html).
""")

print("Part 7 done")

# ─────────────────────────────────────────────────────────────────────────────
# PART 8 — Complete 3D Game
# ─────────────────────────────────────────────────────────────────────────────
P8 = "8-3d-game"

chapter(P8, "01-fps-intro.md", "FPS Introduction", 1, """
# FPS Introduction

`samples/fps_game_template/` demonstrates a minimal **first-person arena**:

- `FlyCamera` with cursor capture
- Lit PBR cubes and ground plane
- LMB spawns emissive tracer bullets
- Custom ray-sphere hitscan (no physics middleware for shooting)

## Class Design: `FpsGame`

```cpp
class FpsGame final : public Game {
    FlyCamera camera{};
    SharedPtr<Mesh> unitCube{};
    SharedPtr<Mesh> groundMesh{};
    Array<GameObject*> targets{};
    Array<TracerBullet> tracers{};
    std::uint32_t shotsFired = 0, hits = 0;
};

struct TracerBullet {
    GameObject* go = nullptr;
    Vector3 velocity{};
    float timeLeft = 0.0F;
};
```

Next: [Project Setup](8-3d-game/02-project-setup.html).
""")

chapter(P8, "02-project-setup.md", "Project Setup", 2, """
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

Next: [Arena and Targets](8-3d-game/03-arena.html).
""")

chapter(P8, "03-arena.md", "Arena and Targets", 3, """
# Arena and Targets

## Ground

```cpp
GameObject* ground = world.CreateGameObject();
ground->AddComponent<TransformComponent>();
ground->AddComponent<MeshComponent>(groundMesh, SceneMeshSlot::Custom,
    Vector3{0.35F, 0.38F, 0.42F});
auto* gmat = ground->AddComponent<MaterialComponent>(nullptr);
gmat->SetRoughness(0.85F);
gmat->SetMetallic(0.0F);
```

## Targets

```cpp
GameObject* target = world.CreateGameObject();
auto* tr = target->AddComponent<TransformComponent>();
tr->SetTranslation({x, 1.0F, z});
tr->SetScale({1.2F, 1.2F, 1.2F});

target->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::Custom, Vector3{0.9F, 0.25F, 0.2F});
auto* mat = target->AddComponent<MaterialComponent>(nullptr);
mat->SetMetallic(0.1F);
mat->SetRoughness(0.4F);
mat->SetEmissive({1.0F, 0.3F, 0.1F});
mat->SetEmissiveStrength(1.5F);

targets.PushBack(target);
```

## Spatial Culling

```cpp
GetScene().SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);
```

Improves frustum culling for many static meshes.

Next: [Shooting and Tracers](8-3d-game/04-shooting.html).
""")

chapter(P8, "04-shooting.md", "Shooting and Tracers", 4, """
# Shooting and Tracers

## Camera Update

```cpp
void FpsGame::OnUpdate(const FrameTiming& timing, IEngineContext& context) {
    IInput& in = context.GetInput();
    if (in.IsCursorCaptured()) {
        camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        camera.ProcessMovement(in, timing.deltaTimeSeconds);
    }
    if (in.IsMouseButtonPressedThisFrame(0))
        TryShootTarget(context);
    UpdateTracerBullets(timing.deltaTimeSeconds);
    Game::OnUpdate(timing, context);
}
```

## Hitscan Ray vs Sphere

```cpp
void TryShootTarget(IEngineContext& context) {
    const Vector3 origin = camera.position;
    const Vector3 dir = camera.Forward();
    ++shotsFired;

    float bestT = 1e9F;
    GameObject* best = nullptr;
    for (GameObject* t : targets) {
        Vector3 center = t->GetComponent<TransformComponent>()->GetWorldPosition();
        float hitT = RaySphereNearestT(origin, dir, center, 0.6F);
        if (hitT > 0.0F && hitT < bestT) { bestT = hitT; best = t; }
    }
    if (best) ++hits;
    SpawnTracerBullet(origin, dir);
}
```

## Tracer Entity

```cpp
void SpawnTracerBullet(const Vector3& origin, const Vector3& dirUnit) {
    GameObject* go = GetWorld().CreateGameObject();
    go->AddComponent<TransformComponent>()->SetTranslation(origin);
    go->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::Custom, Vector3::One);
    auto* mat = go->AddComponent<MaterialComponent>(nullptr);
    mat->SetEmissive({1.0F, 0.9F, 0.4F});
    mat->SetEmissiveStrength(4.0F);

    TracerBullet tb{};
    tb.go = go;
    tb.velocity = dirUnit * 48.0F;
    tb.timeLeft = 0.35F;
    tracers.PushBack(tb);
}
```

Next: [Rendering the 3D Scene](8-3d-game/05-rendering.html).
""")

chapter(P8, "05-rendering.md", "Rendering the 3D Scene", 5, """
# Rendering the 3D Scene

## Full OnRender

```cpp
void FpsGame::OnRender(IRenderFrame&, IEngineContext& context) {
    int fbW = 0, fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    if (fbW <= 0) fbW = 1;
    if (fbH <= 0) fbH = 1;

    const float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(72.0F), aspect, 0.1F, 200.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    Vector3 pr{}, pu{};
    camera.BillboardBasis(pr, pu);

    SubmitStandardLitSceneFromWorld(
        GetWorld(), context, viewProj, camera.position,
        Vector3{0.28F, -1.0F, 0.18F}.Normalized(),
        Vector3{1.0F, 0.97F, 0.92F}, 3.2F,
        Vector3{0.10F, 0.12F, 0.18F},
        true, pr, pu, sceneTimeSeconds,
        SceneSpriteSortMode::SortOrderOnly);
}
```

## HUD Overlay

```cpp
hudText->SetText(Utf8String(std::format(
    "FPS template | shots {} hits {} | LMB fire | WASD move | ESC quit",
    shotsFired, hits).c_str()));
```

`TextOverlayComponent` is collected by `FillStandardLitSceneFromWorld`.

Next: [Extending the FPS](8-3d-game/06-extending.html).
""")

chapter(P8, "06-extending.md", "Extending the FPS", 6, """
# Extending the FPS

## Weapons System

```cpp
struct Weapon {
    float fireRateHz = 8.0F;
    float spreadRadians = 0.02F;
    float cooldownLeft = 0.0F;
};
```

Tick cooldown in `OnUpdate`; apply random yaw/pitch offset to `dir` before hitscan.

## Enemy AI

```cpp
auto* enemy = world.CreateGameObject();
enemy->AddComponent<TransformComponent>();
auto* agent = enemy->AddComponent<AiAgentComponent>();
agent->SetMaxSpeed(5.0F);
agent->SetSteeringPlane(AiSteeringPlane::XzWorld);
// + FSM chase/attack states
```

Call `SimulateGameAi` after player movement.

## Physics Projectiles

Replace tracers with:

```cpp
go->AddComponent<SphereCollider3DComponent>(0.1F);
go->AddComponent<Rigidbody3DComponent>()->SetVelocity(dir * 30.0F);
SimulatePhysics3D(world, timing, settings);
```

## glTF Weapons + Skinned Hands

```cpp
SkinnedGltfAsset arms = world.LoadSkinnedGltf("assets/models/arms.glb");
// SkinnedMeshComponent + AnimatorComponent on viewmodel entity
// Parent to camera with offset transform
```

## C# Scripting

Spark supports CoreCLR scripting (`SPARK_BUILD_SCRIPT_HOST`) — see `docs/CSHARP_SCRIPTING.md`.

---

**Congratulations!** You have completed all eight parts. Explore `SparkDemo` demos and `docs/ARCHITECTURE_AND_DEVELOPER_GUIDE.md` for advanced topics: scene editor, deferred paths, open-world roadmap.
""")

print("Part 8 done")
print("Ebook generation complete — 45 chapters + index")
