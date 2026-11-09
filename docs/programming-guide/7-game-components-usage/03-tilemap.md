# Component Guide: Tilemap

Tilemap components implement **grid storage**, **collision**, **pathfinding**, **TMX import**, and **object spawns**. Rendering uses `TilemapComponent` → `VulkanTilemapPass`.

**Related:** [Tilemaps](../2-2d-graphics/03-tilemaps.md), **[Tilemap level authoring](../2-2d-graphics/08-tilemap-level-authoring.md)**, [Pathfinding](../4-ai/05-pathfinding.md).

| Component | Role |
|-----------|------|
| `TilemapComponent` | Layers + cells |
| `TilemapCollider2DComponent` | Physics bake |
| `TilemapGameplayGridComponent` | Walkability grid |
| `TilemapTileAnimatorComponent` | Animated tiles |
| `TilemapAutotileComponent` | Terrain autotile |
| `TilemapMapSourceComponent` | TMX / hot reload |
| `TilemapObjectLayerComponent` | Markers |
| `TilemapObjectSpawnComponent` | Spawn registry |
| `TilemapObjectGizmoComponent` | Editor gizmos |

---

## TilemapComponent

**Header:** `spark/ecs/components/rendering/TilemapComponent.hpp`

#### C++

```cpp
SharedPtr<Tileset> tileset = CreateTilesetFromAtlas(atlas, 16, 16);
auto* tm = mapGo->AddComponent<TilemapComponent>(tileset, 64, 32, 1.0F, 0);
tm->AddLayer("Terrain");
tm->SetTileCell(0, 10, 4, TileCell::FromTileId(wallId));
```

#### C\#

```csharp
var tm = mapGo.AddTilemap(tileset, 64, 32, 1f, 0);
tm.AddLayer("Terrain");
```

#### Python

```python
tm = map_go.add_tilemap("tilesets/dungeon", 64, 32, tile_world_size=1.0, sort_order_base=0)
```

---

## TilemapCollider2DComponent

Bakes static colliders from `TileDefinition::collisionShape` on layers with `contributeCollision`.

#### C++

```cpp
mapGo->AddComponent<TilemapCollider2DComponent>()->Rebuild(*mapGo);
```

#### C\#

```csharp
mapGo.GetOrAddComponent<TilemapCollider2DComponent>()?.Rebuild(mapGo);
```

#### Python

```python
map_go.get_or_add_default_component(ComponentKind.TilemapCollider2D)
```

---

## TilemapGameplayGridComponent

#### C++

```cpp
auto* grid = mapGo->AddComponent<TilemapGameplayGridComponent>();
grid->SetWalkRule(TilemapGameplayWalkRule::DefinitionAndFlags);
grid->SetAutoRebake(true);
grid->RequestRebake();
```

#### C\#

```csharp
var grid = mapGo.AddTilemapGameplayGrid();
grid.SetAutoRebake(true);
```

#### Python

```python
map_go.get_or_add_default_component(ComponentKind.TilemapGameplayGrid)
```

---

## TilemapTileAnimatorComponent

#### C++

```cpp
mapGo->AddComponent<TilemapTileAnimatorComponent>()->SetGlobalTimeScale(1.0F);
```

#### C\#

```csharp
mapGo.GetOrAddComponent<TilemapTileAnimatorComponent>();
```

#### Python

```python
map_go.get_or_add_default_component(ComponentKind.TilemapTileAnimator)
```

---

## TilemapAutotileComponent

#### C++

```cpp
auto* autoTile = mapGo->AddComponent<TilemapAutotileComponent>();
autoTile->SetPaintLayerIndex(0);
autoTile->SetAutotileGroup(1);
```

#### C\#

```csharp
mapGo.GetOrAddComponent<TilemapAutotileComponent>()?.SetPaintLayerIndex(0);
```

#### Python

```python
map_go.get_or_add_default_component(ComponentKind.TilemapAutotile)
```

---

## TilemapMapSourceComponent

#### C++

```cpp
auto* src = mapGo->AddComponent<TilemapMapSourceComponent>();
src->SetTmxPath("sprites/kenney_tiny-dungeon/Tiled/sampleMap.tmx");
src->SetPixelsPerWorldUnit(16.0F);
src->ImportNow(*mapGo, world);
```

#### C\#

```csharp
src.SetTmxPath("sprites/kenney_tiny-dungeon/Tiled/sampleMap.tmx");
src.ImportNow(mapGo, world);
```

#### Python

```python
src = map_go.add_tilemap_map_source()  # when mirrored
src.set_pixels_per_world_unit(16.0)
```

---

## TilemapObjectLayerComponent + spawns

#### C++

```cpp
auto* objects = mapGo->AddComponent<TilemapObjectLayerComponent>();
uint32_t li = objects->AddObjectLayer("Spawns");
TilemapObjectMarker m{};
m.typeId = Utf8String("player_spawn");
m.cellX = 4; m.cellY = 2;
objects->AddMarker(li, m);
mapGo->AddComponent<TilemapObjectSpawnComponent>();
```

#### C\#

```csharp
mapGo.AddTilemapObjectSpawn();
```

#### Python

```python
map_go.get_or_add_default_component(ComponentKind.TilemapObjectSpawn)
```

---

## TilemapObjectGizmoComponent

Editor-style visualization of object markers — used in tilemap showcase and editor services.

#### C++

```cpp
mapGo->AddComponent<TilemapObjectGizmoComponent>();
```

#### C\#

```csharp
mapGo.GetOrAddComponent<TilemapObjectGizmoComponent>();
```

#### Python

```python
map_go.get_or_add_default_component(ComponentKind.TilemapObjectGizmo)
```

Next: [Lighting components](04-lighting.md).
