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

Next: [Camera2D](02-camera2d.md).
