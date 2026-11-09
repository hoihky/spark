# Building the Level

`Platformer2DDemo` builds side-view geometry with **scaled sprites + static rigidbodies**. Top-down and grid games use **tilemaps** (`TilemapShowcase2DDemo` **#19**, P0 path **F**). Component recipes: [Part 7 — Tilemaps](../7-game-components-usage/03-tilemap.md).

## Concept: procedural platforms (`AddSolidPlatform`)

Each platform is one `GameObject` with transform, sprite, box collider, and static body — same pattern as the demo helper.

### C++

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

### C\#

```csharp
static GameObject AddSolidPlatform(GameWorld w, Texture2D tex,
    float x0, float y0, float x1, float y1, int sortOrder, Vector4 tint)
{
  var go = w.CreateGameObject();
  var tr = go.GetOrAddTransform();
  tr.SetTranslation(new Vector3((x0 + x1) * 0.5f, (y0 + y1) * 0.5f, 0.01f + sortOrder * 0.0004f));
  tr.SetScale(new Vector3(MathF.Abs(x1 - x0), MathF.Abs(y1 - y0), 1f));
  go.AddSprite(tex, tint, new Vector4(0, 0, 1, 1), sortOrder);
  go.AddBoxCollider2D();
  go.AddRigidbody2D(RigidbodyBodyType2D.Static, 0f);
  return go;
}
```

### Python

```python
from spark._generated.component_kind import ComponentKind

def add_solid_platform(world, tex_key, x0, y0, x1, y1, sort_order, tint):
    go = world.create_game_object()
    tr = go.get_or_add_transform()
    tr.translation = Vector3((x0 + x1) * 0.5, (y0 + y1) * 0.5, 0.01 + sort_order * 0.0004)
    tr.scale = Vector3(abs(x1 - x0), abs(y1 - y0), 1.0)
    go.add_sprite(tex_key, tint, Vector4(0, 0, 1, 1), sort_order=sort_order)
    go.get_or_add_default_component(ComponentKind.BoxCollider2D)
    go.get_or_add_default_component(ComponentKind.Rigidbody2D)
```

**Draw order:** bump `sortOrder` per platform so Z fights never occur among floor pieces.

## Concept: checker tile texture (demo bootstrap)

### C++

```cpp
tileTex = MakeShared<Texture2D>(Utf8String("Tiles"));
*tileTex = Texture2D::CreateCheckerboard(256, 32,
    Vector3{0.38F, 0.34F, 0.30F}, Vector3{0.16F, 0.48F, 0.30F});
world.RegisterTexture(tileTex, "platformer2d_template/tiles");

AddSolidPlatform(world, roots, tileTex, -12, -3, 24, kGroundTopY, 10, ...);
```

### C\#

```csharp
world.RegisterCheckerboardTexture("platformer2d_template/tiles", c0, c1, 256, 32);
AddSolidPlatform(world, tex, -12, -3, 24, groundTopY, 10, Vector4.One);
```

### Python

```python
world.register_checkerboard_texture(
    "platformer2d_template/tiles",
    Vector3(0.38, 0.34, 0.30),
    Vector3(0.16, 0.48, 0.30),
)
```

---

## Tilemap levels (production path)

For grid-based levels, use `TilemapComponent` with stacked layers, `TilemapCollider2DComponent`, and optional `TilemapGameplayGridComponent` for AI pathfinding. SparkDemo **#19** loads Kenney `sampleMap.tmx` at runtime.

**Full level pipeline (TMX → play):** [Tilemap level authoring](../2-2d-graphics/08-tilemap-level-authoring.md). Primitives: [Tilemaps](../2-2d-graphics/03-tilemaps.md).

### Collision layer on tiles

Enable `contributeCollision` on terrain layers and set per-tile `collisionShape` on the `Tileset`. Add `TilemapCollider2DComponent` on the map object so `PhysicsWorld2D` resolves player/NPC bodies against baked statics.

#### C++

```cpp
auto* collider = level->AddComponent<TilemapCollider2DComponent>();
collider->Rebuild(*level);
```

#### C\#

```csharp
level.GetComponent<TilemapCollider2DComponent>()?.Rebuild(level);
```

#### Python

```python
level.get_or_add_default_component(ComponentKind.TilemapCollider2D)
# Rebuild when native interop exposes tilemap_collider_2d.rebuild(level)
```

### Gameplay grid (pathfinding / spawn validation)

#### C++

```cpp
auto* grid = levelRoot->AddComponent<TilemapGameplayGridComponent>();
grid->SetWalkRule(TilemapGameplayWalkRule::DefinitionAndFlags);
grid->SetAutoRebake(true);
```

#### C\#

```csharp
var grid = levelRoot.AddTilemapGameplayGrid();
grid.SetWalkRule(TilemapGameplayWalkRule.DefinitionAndFlags);
grid.SetAutoRebake(true);
```

#### Python

```python
grid = level_root.get_or_add_default_component(ComponentKind.TilemapGameplayGrid)
# set_walk_rule / set_auto_rebake when bound in spark-python
```

Use `GetGridFrame()` for cell ↔ world conversion when placing markers or validating spawn cells.

### Object layers (gems, enemies, goals)

Markers live in `TilemapObjectLayerComponent` (not painted tiles). Register spawn handlers and use `TilemapObjectSpawnComponent::RespawnAll`:

#### C++

```cpp
TilemapObjectSpawnRegistry::Default().Register("p0_gem", &SpawnGemHandler);
level->AddComponent<TilemapObjectSpawnComponent>()->RespawnAll(*level, world);
```

#### C\#

```csharp
TilemapObjectSpawnRegistry.Default.Register("p0_gem", SpawnGemHandler);
level.GetComponent<TilemapObjectSpawnComponent>()?.RespawnAll(level, world);
```

#### Python

```python
# Handlers typically registered from C++ gameplay module today;
# spawn via get_or_add_default_component(ComponentKind.TilemapObjectSpawn)
```

Full walkthrough: [Gameplay API — object layers](09-2d-gameplay-api-guide.md#5-object-layers-markers-and-spawning). P0 demo: shell **F**.

### Autotile + animated tiles

#### C++

```cpp
#include "spark/ecs/components/tilemap/TilemapAutotileComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapTileAnimatorComponent.hpp"

auto* autotile = level->AddComponent<TilemapAutotileComponent>();
autotile->SetTerrainLayerIndex(0U);
autotile->RequestRebuild();

auto* anim = level->AddComponent<TilemapTileAnimatorComponent>();
anim->SetAdvanceSceneTime(true);
```

#### C\#

```csharp
level.AddTilemapAutotile().SetTerrainLayerIndex(0).RequestRebuild();
level.AddTilemapTileAnimator().SetAdvanceSceneTime(true);
```

#### Python

```python
level.get_or_add_default_component(ComponentKind.TilemapAutotile)
level.get_or_add_default_component(ComponentKind.TilemapTileAnimator)
```

### Debug: `TilemapObjectGizmoComponent`

```cpp
level->AddComponent<TilemapObjectGizmoComponent>();
```

Editor-style demos draw object-layer markers in-world without a separate tool.

## Level root checklist

| Component | Required? |
|-----------|-----------|
| `TilemapMapSourceComponent` | If loading TMX |
| `TilemapComponent` | Yes (tilemap path) |
| `TilemapGameplayGridComponent` | For AI / spawn validation |
| `TilemapCollider2DComponent` | For physics blocking |
| `TilemapObjectLayer` + `TilemapObjectSpawn` | For registered spawns |

More types: [10 — Components handbook](10-2d-components-handbook.md#tilemapgameplaygrid).

Next: [Player Controller](04-player-controller.md).
