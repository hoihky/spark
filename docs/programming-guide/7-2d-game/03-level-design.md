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

## Tilemaps (alternative to sprite platforms)

For grid-based levels, use `TilemapComponent` with stacked layers, `TilemapCollider2DComponent`, and optional `TilemapGameplayGridComponent` for AI pathfinding. SparkDemo **#19** loads Kenney `sampleMap.tmx` at runtime. See [Tilemaps](../2-2d-graphics/03-tilemaps.md).

### Collision layer on tiles

Enable `contributeCollision` on terrain layers and set per-tile `collisionShape` on the `Tileset`. Add `TilemapCollider2DComponent` on the map object so `PhysicsWorld2D` resolves player/NPC bodies against baked statics.

### Gameplay grid (pathfinding / spawn)

```cpp
auto* grid = levelRoot->AddComponent<TilemapGameplayGridComponent>();
grid->SetWalkRule(TilemapGameplayWalkRule::DefinitionAndFlags);
grid->SetAutoRebake(true);
```

Use `GetGridFrame()` for cell ↔ world conversion when placing markers or validating spawn cells.

### Object layers (gems, enemies, goals)

Markers live in `TilemapObjectLayerComponent` (not painted tiles). Register spawn handlers and use `TilemapObjectSpawnComponent::RespawnAll`:

```cpp
TilemapObjectSpawnRegistry::Default().Register("p0_gem", &SpawnGemHandler);
```

Full walkthrough: [Gameplay API — object layers](09-2d-gameplay-api-guide.md#5-object-layers-markers-and-spawning). P0 demo: shell **F**.

### `TilemapAutotileComponent` + `TilemapTileAnimatorComponent`

```cpp
#include "spark/ecs/components/tilemap/TilemapAutotileComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapTileAnimatorComponent.hpp"

auto* autotile = level->AddComponent<TilemapAutotileComponent>();
autotile->SetTerrainLayerIndex(0U);
autotile->RequestRebuild();

auto* anim = level->AddComponent<TilemapTileAnimatorComponent>();
anim->SetAdvanceSceneTime(true);
```

### `TilemapObjectGizmoComponent`

Debug-draw markers in editor-style demos:

```cpp
level->AddComponent<TilemapObjectGizmoComponent>();
```

### Component checklist (level root)

| Component | Required? |
|-----------|-----------|
| `TilemapMapSourceComponent` | If loading TMX |
| `TilemapComponent` | Yes |
| `TilemapGameplayGridComponent` | For AI / spawn validation |
| `TilemapCollider2DComponent` | For physics blocking |
| `TilemapObjectLayer` + `TilemapObjectSpawn` | For registered spawns |

More types: [10 — Components handbook](10-2d-components-handbook.md#tilemapgameplaygrid).

Next: [Player Controller](04-player-controller.md).
