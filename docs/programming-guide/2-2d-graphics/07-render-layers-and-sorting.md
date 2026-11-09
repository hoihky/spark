# Render layers and sorting (top-down 2D)

SparkDemo **#18 — 2D render layers** (`RenderLayers2DDemo`) shows how to stack **background / characters / effects** using named render layers, optional **sorting groups**, and **`SceneSpriteSortMode::SortOrderThenWorldY`** for ARPG-style depth.

Related: [Sprites](01-sprites.md) · [2D render pipeline](06-2d-render-pipeline.md) · [2D ARPG features](../../2D_ARPG_FEATURES.md) (P1.1 Y-sort).

## When to use what

| Mechanism | Solves |
|-----------|--------|
| `SpriteComponent::sortOrder` | Global draw priority within a pass |
| `RenderLayerComponent` | Mask which logical layer a drawable belongs to (Background / Characters / Effects) |
| `SortingGroupComponent` | Sort **children as one unit** (shadow → body → hat) |
| `SceneSpriteSortMode::SortOrderThenWorldY` | Lower world **Y draws on top** (+Y up) for same sort band |
| `TilemapLayer::orderInLayerOffset` | Tile layers vs sprites ordering ([Tilemaps](03-tilemaps.md)) |

Platformers often use **sort order + fixed Z**; top-down / farming ARPGs combine **render layers + world Y**.

## Concept: register render layers

Layer names are registered once (demo registers in `Load`):

### C++

```cpp
#include "spark/render/sprites2d/RenderLayerRegistry.hpp"
#include "spark/ecs/components/rendering/RenderLayerComponent.hpp"

RenderLayerRegistry& layers = RenderLayerRegistry::Instance();
const RenderLayerId bgId = layers.FindLayerIdByName("Background");
const RenderLayerId charId = layers.FindLayerIdByName("Characters");
const RenderLayerId fxId = layers.FindLayerIdByName("Effects");

farm->AddComponent<RenderLayerComponent>(bgId);
player->AddComponent<RenderLayerComponent>(charId);
sparkle->AddComponent<RenderLayerComponent>(fxId);
```

### C\#

```csharp
var bgId = RenderLayerRegistry.Instance.FindLayerIdByName("Background");
farmer.AddRenderLayer(charId);
```

### Python

```python
from spark._generated.component_kind import ComponentKind
go.get_or_add_default_component(ComponentKind.RenderLayer)
```

If a name is missing, register it in your bootstrap before attaching components (see demo `RenderLayers2DDemo::Load`).

## Concept: sorting group (character stack)

Without a group, shadow, body, and hat sprites each participate in **global** Y-sort — feet and hat can interleave with props incorrectly.

### C++

```cpp
#include "spark/ecs/components/rendering/SortingGroupComponent.hpp"

GameObject* groupedRoot = world.CreateGameObject();
groupedRoot->AddComponent<RenderLayerComponent>(charactersLayer);
groupedRoot->AddComponent<SortingGroupComponent>(100);  // group sort offset

// Parent shadow, body, hat as children (or same object with local Z)
shadowGo->SetParent(groupedRoot);
bodyGo->SetParent(groupedRoot);
hatGo->SetParent(groupedRoot);
```

The group’s combined bounds sort against other farmers and props; **inside** the group, use local `sortOrder` / child Z.

### C\#

```csharp
root.AddSortingGroup(100);
```

### Python

```python
root.get_or_add_default_component(ComponentKind.SortingGroup)
```

Demo **Q** toggles shadow visibility to show broken vs grouped sorting.

## Concept: submit with Y-sort

### C++

```cpp
SubmitStandardLitSceneFromWorld(
    world, context, viewProj, cameraPos,
    sunDir, sunColor, sunIntensity, ambient,
    enableParticles, pr, pu, sceneTimeSeconds,
    SceneSpriteSortMode::SortOrderThenWorldY);
```

### C\#

```csharp
SceneSubmit.SubmitStandardLitSceneFromWorld(
    World, context, viewProj, cameraPos,
    sunDir, sunColor, sunIntensity, ambient,
    enableSprites: true, sceneTime,
    sortMode: SceneSpriteSortMode.SortOrderThenWorldY);
```

### Python

```python
# Pass sort_mode when native submit wrapper exposes it; else host C++ game.
```

Default `SortOrderOnly` is fine for pure platformers with explicit `sortOrder` per platform.

## Concept: tilemap layers vs sprites

Terrain tilemaps usually sit in **Background** with low `sortOrderBase`. Characters use **Characters** layer + Y-sort. Foreground vines can use higher `orderInLayerOffset` on a `TilemapLayer` or a foreground sprite layer.

| Content | Layer | Sort |
|---------|-------|------|
| Ground tiles | Tilemap layer 0 | `sortOrderBase` low |
| Props (sprites) | `RenderLayerComponent` Background | Y-sort |
| Player / NPC | Characters + optional `SortingGroup` | Y-sort |
| Hit sparks | Effects | Y-sort or high `sortOrder` |

## Debugging draw order

| Symptom | Check |
|---------|--------|
| Character walks **behind** tree they should cover | Y-sort enabled? Character feet Y vs prop pivot Y |
| Hat draws under shadow | Missing `SortingGroupComponent` on root |
| Layer never visible | Wrong `RenderLayerId` or layer culled in submit |
| Tilemap always on top | `orderInLayerOffset` vs sprite `sortOrder` |

Try demo **#18**, press **Q**, compare left farmer (ungrouped) vs right farmer (grouped).

Next: [Tilemap level authoring](08-tilemap-level-authoring.md).
