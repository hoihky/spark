---
title: Tilemaps
order: 3
---

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
