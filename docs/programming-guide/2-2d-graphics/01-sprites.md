# Sprites

Sprites are the primary **world-space 2D drawable** in Spark. Each `SpriteComponent` describes a textured unit quad in the XY plane; the Vulkan **sprite pass** instancing path turns thousands of them into a single batched draw while respecting tint, atlas UVs, optional 2D lighting, and sort order.

**Try it:** SparkDemo launcher **Hello** / Python `hello_game.py` (checkerboard triangle), **Platformer2D**, and **Tilemap Showcase** for layered stacks.

## Mental model

```mermaid
flowchart LR
  ECS[SpriteComponent + Transform] --> Fill[FillStandardLitSceneFromWorld]
  Fill --> Params[SceneRenderParams.sprites]
  Params --> Vulkan[VulkanSpritePass batch]
  Vulkan --> Composite[HDR scene + 2D composite]
```

1. **Authoring:** attach `SpriteComponent` on a `GameObject` with `TransformComponent`.
2. **Registration:** textures live in `GameWorld` under string keys (or inline `SharedPtr<Texture2D>` on the component).
3. **Submit:** `FillStandardLitSceneFromWorld` walks active objects and appends `SceneSpriteDraw` records.
4. **GPU:** the renderer maps each draw to a texture **layer index** (max **16** unique atlases per frame) and sorts by `SceneSpriteSortMode`.

World space is **+Y up**. The orthographic projection used with `Camera2D` flips framebuffer Y so screen coordinates still grow downward — do not double-apply an OpenGL-style Y flip when picking or placing HUD.

## Class design: `SpriteComponent`

Header: `spark/ecs/components/SpriteComponent.hpp`.

| Member / API | Role |
|--------------|------|
| `SetTexture(SharedPtr<Texture2D>)` | CPU RGBA8 atlas or solid color |
| `SetTint(Vector4)` | Per-sprite multiply (RGBA) |
| `SetUvRect(Vector4)` | Atlas bounds `(minU, minV, maxU, maxV)` in 0–1 |
| `SetSortOrder(int32_t)` | Draw priority — **higher draws later** (on top) |
| `GetTexture()` | Current atlas pointer |

The mesh is always a **1×1 unit quad** centered on the transform origin in XY (`z` is usually a small positive bias to order against tilemaps).

Constructor:

```cpp
SpriteComponent(SharedPtr<Texture2D> inTexture, const Vector4& inTint,
                const Vector4& inUvRect, std::int32_t inSortOrder) noexcept;
```

Pair with optional `SpriteLighting2DComponent` for lit modes (see [2D Lighting](05-2d-lighting.md)).

## Class design: `Texture2D`

CPU-side RGBA8 image uploaded when referenced from `SceneRenderParams::sceneTextures`.

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

**Library keys:** `world.Assets().RegisterTexture(tex, "game/hero")` lets multiple sprites share one GPU upload. Scripting hosts typically call `RegisterCheckerboardTexture` / `LoadTexture` interop helpers (C#: `world.Assets` / `RegisterTextureFromFile`) with the same key string you pass to `AddSprite`.

## Concept: spawn a sprite from a texture key

### C++

```cpp
auto tex = MakeShared<Texture2D>(Utf8String("Hero"));
if (!Texture2D::TryLoadFromFile("assets/sprites/hero.png", *tex)) {
    *tex = Texture2D::CreateSolid(64, 64, Vector4{1, 0, 0, 1});
}
world.Assets().RegisterTexture(tex, "game/hero");

GameObject* go = world.CreateGameObject();
TransformComponent* tr = go->AddComponent<TransformComponent>();
tr->SetTranslation({2.0F, 1.0F, 0.04F});
tr->SetUniformScale(1.2F);

go->AddComponent<SpriteComponent>(
    tex,
    Vector4{1.0F, 1.0F, 1.0F, 1.0F},
    Vector4{0.0F, 0.0F, 1.0F, 1.0F},
    100);
```

### C\#

From a `GameObject` mirror (`CppMirrors.g.cs`):

```csharp
world.Assets.RegisterTextureFromFile("game/hero", "assets/sprites/hero.png");

var go = world.CreateGameObject("Hero");
go.GetOrAddTransform().SetTranslation(new Vector3(2f, 1f, 0.04f));
go.GetOrAddTransform().SetUniformScale(1.2f);

var tint = new Vector4(1f, 1f, 1f, 1f);
var uv = new Vector4(0f, 0f, 1f, 1f);
go.AddSprite("game/hero", tint, uv, sortOrder: 100);
```

### Python

```python
from spark import Vector3, Vector4

world.register_checkerboard_texture("hello/triangle", Vector3(0.95, 0.55, 0.15), Vector3(0.15, 0.18, 0.22))

triangle = world.create_game_object("Triangle")
triangle.get_or_add_transform().set_uniform_scale(3.0)
triangle.add_sprite(
    "hello/triangle",
    Vector4(1.0, 1.0, 1.0, 1.0),
    Vector4(0.0, 0.0, 1.0, 1.0),
    sort_order=0,
)
```

## Concept: sprite sheet / atlas UVs

Atlas animation is **UV animation** on the same texture. Grid helpers:

```cpp
void SetFrame(SpriteComponent* sprite, int col, int row, int cols, int rows) {
    const float du = 1.0F / static_cast<float>(cols);
    const float dv = 1.0F / static_cast<float>(rows);
    sprite->SetUvRect({col * du, row * dv, (col + 1) * du, (row + 1) * dv});
}
```

### C++

```cpp
SetFrame(sprite, 2, 1, 4, 4);  // third column, second row in a 4×4 grid
```

### C\#

```csharp
var sprite = go.GetComponent<SpriteComponent>();
float du = 1f / 4f, dv = 1f / 4f;
int col = 2, row = 1;
sprite.SetUvRect(new Vector4(col * du, row * dv, (col + 1) * du, (row + 1) * dv));
```

### Python

```python
sprite = triangle.get_component_sprite()
du = dv = 0.25
col, row = 2, 1
sprite.set_uv_rect(Vector4(col * du, row * dv, (col + 1) * du, (row + 1) * dv))
```

For clip-based playback use `SpriteAnimatorComponent` instead of manual UV stepping ([2D Animation](04-2d-animation.md)).

## Concept: sort layers and depth bias

| Typical `sortOrder` | Layer |
|---------------------|-------|
| 0–20 | Far background / sky |
| 50–90 | Parallax hills |
| 100–150 | Gameplay characters & props |
| 200+ | VFX, weapon flashes |
| N/A | HUD → `TextOverlayComponent` or GUI canvas (screen space) |

Use a small positive **Z translation** (e.g. `0.02`–`0.08`) when you need stable ordering among sprites with the same `sortOrder` and Y-sort is enabled.

### C++

```cpp
bgSprite->SetSortOrder(10);
playerSprite->SetSortOrder(100);
playerTr->SetTranslation({x, y, 0.05F});
```

### C\#

```csharp
bg.AddSprite(..., sortOrder: 10);
player.AddSprite(..., sortOrder: 100);
player.GetOrAddTransform().SetTranslation(new Vector3(x, y, 0.05f));
```

### Python

```python
bg.add_sprite(tex_key, tint, uv, sort_order=10)
player.add_sprite(tex_key, tint, uv, sort_order=100)
player.get_or_add_transform().set_translation(Vector3(x, y, 0.05))
```

## Concept: flip without mirror textures

Negative scale on X mirrors the quad; UVs stay the same.

### C++

```cpp
const bool facingLeft = velocity.x < 0.0F;
tr->SetScale({facingLeft ? -baseScaleX : baseScaleX, baseScaleY, 1.0F});
```

### C\#

```csharp
var tr = go.GetOrAddTransform();
float sx = facingLeft ? -Math.Abs(baseScaleX) : Math.Abs(baseScaleX);
tr.SetScale(new Vector3(sx, baseScaleY, 1f));
```

### Python

```python
sx = -abs(base_scale_x) if facing_left else abs(base_scale_x)
go.get_or_add_transform().set_scale(Vector3(sx, base_scale_y, 1.0))
```

## Advanced: manual `SceneSpriteDraw`

When building custom render paths (tools, unit tests, alternate games), you can bypass ECS collection and write draws directly:

```cpp
SceneRenderParams params{};
SceneSpriteDraw draw{};
draw.model = transform->GetWorldMatrix();
draw.tint = sprite->GetTint();
draw.uvRect = sprite->GetUvRect();
draw.sortOrder = sprite->GetSortOrder();
draw.textureLayer = world.ResolveTextureLayer(sprite->GetTexture());
params.sprites.PushBack(draw);
context.SetSceneRenderParams(params);
```

Production games should prefer `FillStandardLitSceneFromWorld` so tilemaps, lights, particles, and composite views stay consistent ([2D Render Pipeline](06-2d-render-pipeline.md)).

## Limits and debugging

| Limit | Value | Where |
|-------|-------|--------|
| Sprites per frame | 8192 | `SceneRenderParams::sprites` |
| Texture layers | 16 | `sceneTextures` |
| UI texture layers | separate budget | `uiTextures` |

If sprites disappear, check: texture not registered → `textureLayer` invalid; exceeded 16 unique atlases → last wins or drops; `sortOrder` under background layer; alpha zero tint.

See [2D runtime limits](../8-2d-game/08-scene2d-runtime-limits.md) for composite capture and HDR specifics.

Next: [Camera2D](02-camera2d.md).
