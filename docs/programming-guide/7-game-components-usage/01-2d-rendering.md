# Component Guide: 2D Rendering

This chapter explains **ECS components that feed the 2D draw path** (`SceneSpriteDraw`, tilemap passes, screen overlays, and sprite-layer particles). Pair each component with `TransformComponent` unless noted.

**Related:** [Sprites](../2-2d-graphics/01-sprites.md), [2D render pipeline](../2-2d-graphics/06-2d-render-pipeline.md), [Field reference](../1-overview-architecture/07-game-component-reference.md).

**Python:** `from spark._generated.component_kind import ComponentKind` for `get_or_add_default_component`; typed helpers live in `spark/game_object.py` and `spark/components/` (regenerate after catalog updates).

| Component | Role |
|-----------|------|
| `TransformComponent` | Pose (required) |
| `SpriteComponent` | Textured quad |
| `SpriteLighting2DComponent` | Lit sprite modes |
| `BlendModeComponent` | Alpha blend equation |
| `RenderLayerComponent` | Logical layer bitmask |
| `SortingGroupComponent` | Group sort offset |
| `TextOverlayComponent` | Screen/world text HUD |
| `ParallaxLayerComponent` | Camera-relative scroll |
| `Scene2DCompositeViewComponent` | Minimap / fog capture |
| `ParticleEmitterComponent` | CPU particles (sprite layer) |
| `VfxPlayerComponent` | `.sparkvfx` one-shots |

---

## TransformComponent

**Header:** `spark/ecs/components/core/TransformComponent.hpp`  
Every drawable needs local TRS; parent chain composes `GameObject::GetWorldMatrix()`.

**When to use:** Always on entities that move, scale, or rotate in the scene.

#### C++

```cpp
TransformComponent* tr = go->AddComponent<TransformComponent>();
tr->SetTranslation({2.0F, 1.0F, 0.05F});
tr->SetUniformScale(1.5F);
tr->SetRotationEuler({0.0F, 0.0F, Spark::DegreesToRadians(15.0F)});
```

#### C\#

```csharp
var tr = go.GetOrAddTransform();
tr.SetTranslation(new Vector3(2f, 1f, 0.05f));
tr.SetUniformScale(1.5f);
```

#### Python

```python
tr = go.get_or_add_transform()
tr.set_uniform_scale(1.5)
tr.translation = Vector3(2.0, 1.0, 0.05)
```

---

## SpriteComponent

**Header:** `spark/ecs/components/SpriteComponent.hpp`  
Unlit alpha-blended unit quad; tint, UV rect, `sortOrder`. Texture via `SharedPtr<Texture2D>` or world-registered key at submit.

**When to use:** Characters, props, UI-in-world, VFX quads.

**Pairs with:** `SpriteLighting2DComponent`, `SpriteAnimatorComponent`, `BlendModeComponent`.

#### C++

```cpp
world.RegisterTexture(albedo, "game/hero");
go->AddComponent<SpriteComponent>(
    albedo,
    Vector4{1, 1, 1, 1},
    Vector4{0, 0, 1, 1},
    /*sortOrder=*/100);
```

#### C\#

```csharp
world.RegisterTextureFromFile("game/hero", "assets/sprites/hero.png");
go.AddSprite("game/hero", new Vector4(1, 1, 1, 1), new Vector4(0, 0, 1, 1), sortOrder: 100);
```

#### Python

```python
world.register_checkerboard_texture("game/hero", c0, c1)  # or load_texture when available
sprite = go.add_sprite("game/hero", Vector4(1, 1, 1, 1), Vector4(0, 0, 1, 1), sort_order=100)
sprite.set_sort_order(120)
```

---

## SpriteLighting2DComponent

**Header:** `spark/ecs/components/rendering/SpriteLighting2DComponent.hpp`  
Selects `SpriteLighting2DMode` (normal map, ramp, rim, hit flash, outline, …). Consumed in `sprite.frag` during `FillStandardLitSceneFromWorld`.

**When to use:** Heroes, torches, gems with pickup flash — not flat background tiles.

**Pairs with:** `SpriteComponent` on the **same** object; optional `PointLight2DComponent` nearby.

#### C++

```cpp
#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"

go->AddComponent<SpriteComponent>(tex, tint, uv, 100);
auto* lit = go->AddComponent<SpriteLighting2DComponent>(
    SpriteLighting2DMode::NormalMapped,
    Vector4{1.0F, 0.35F, 0.9F, 1.0F},
    Vector4{});
lit->SetNormalMap(normalAtlas);
lit->SetSyncNormalUvWithSprite(true);
```

#### C\#

```csharp
go.AddSprite(texKey, tint, uv, 100);
var lit = go.GetOrAddComponent<SpriteLighting2DComponent>();
lit.SetMode(SpriteLighting2DMode.NormalMapped);
lit.SetSyncNormalUvWithSprite(true);
```

#### Python

```python
go.add_sprite(tex_key, tint, uv, sort_order=100)
from spark._generated.component_kind import ComponentKind
lit = go.get_or_add_default_component(ComponentKind.SpriteLighting2D)
# Call generated sprite_lighting_* natives after regenerating spark-python bindings.
```

---

## BlendModeComponent

**Header:** `spark/ecs/components/rendering/BlendModeComponent.hpp`  
Sets `SceneBlendMode` on the resulting `SceneSpriteDraw` (additive glow, multiply shadows).

**When to use:** Additive sparks, multiply ground marks; default alpha blend otherwise.

#### C++

```cpp
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"

go->AddComponent<SpriteComponent>(glowTex, Vector4{1,1,1,0.8F}, fullUv, 200);
go->AddComponent<BlendModeComponent>(SceneBlendMode::Additive);
```

#### C\#

```csharp
go.AddSprite(glowKey, tint, uv, 200);
go.GetOrAddComponent<BlendModeComponent>()?.SetMode(SceneBlendMode.Additive);
```

#### Python

```python
go.add_sprite(glow_key, tint, uv, sort_order=200)
go.get_or_add_default_component(ComponentKind.BlendMode)
```

---

## RenderLayerComponent

**Header:** `spark/ecs/components/rendering/RenderLayerComponent.hpp`  
Assigns a **layer id** used for batching and optional culling masks (with `SceneSpatialPolicyComponent` on the scene root).

**When to use:** Separate “background”, “gameplay”, “overlay” culling in large 2D scenes.

#### C++

```cpp
#include "spark/ecs/components/rendering/RenderLayerComponent.hpp"

go->AddComponent<RenderLayerComponent>()->SetLayerId(1);  // gameplay layer
```

#### C\#

```csharp
go.GetOrAddComponent<RenderLayerComponent>()?.SetLayerId(1);
```

#### Python

```python
layer = go.get_or_add_default_component(ComponentKind.RenderLayer)
# layer.set_layer_id(1) via generated binding
```

---

## SortingGroupComponent

**Header:** `spark/ecs/components/rendering/SortingGroupComponent.hpp`  
Adds a **sort offset** to all drawables on the subtree (children inherit effective order).

**When to use:** Parent “character” object groups body + weapon + shadow sprites with one base order.

#### C++

```cpp
#include "spark/ecs/components/rendering/SortingGroupComponent.hpp"

root->AddComponent<SortingGroupComponent>()->SetSortOrderOffset(1000);
// child sprites use sortOrder 0–10 relative to group
```

#### C\#

```csharp
root.GetOrAddComponent<SortingGroupComponent>()?.SetSortOrderOffset(1000);
```

#### Python

```python
root.get_or_add_default_component(ComponentKind.SortingGroup)
```

---

## TextOverlayComponent

**Header:** `spark/ecs/components/rendering/TextOverlayComponent.hpp`  
Bitmap text in **screen pixels** or anchored to world. Requires `GameWorld::SetUiFont`.

**When to use:** Score, debug labels without full `UiCanvasComponent`.

#### C++

```cpp
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"

MountUiFont(world);
auto* hud = world.CreateGameObject();
auto* text = hud->AddComponent<TextOverlayComponent>();
text->SetScreenPosition(12.0F, 12.0F);
text->SetFontSizePixels(20.0F);
text->SetColor({0.94F, 0.97F, 1.0F});
text->SetText(Utf8String("Score: 0"));
```

#### C\#

```csharp
go.AddTextOverlay();
var to = go.GetComponent<TextOverlayComponent>();
to.SetScreenPosition(12f, 12f);
to.SetText("Score: 0");
```

#### Python

```python
# Prefer native MountUiFont in host; TextOverlay via ComponentKind when bound:
go.get_or_add_default_component(ComponentKind.TextOverlay)
```

---

## ParallaxLayerComponent

**Header:** `spark/ecs/components/rendering/ParallaxLayerComponent.hpp`  
**Update priority 290** — adjusts transform from camera motion using `factorX` / `factorY` and optional sine drift.

**When to use:** Background hills, clouds behind `Camera2DRigComponent`.

#### C++

```cpp
#include "spark/ecs/components/rendering/ParallaxLayerComponent.hpp"

bg->AddComponent<SpriteComponent>(skyTex, tint, uv, 5);
ParallaxLayerComponent* px = bg->AddComponent<ParallaxLayerComponent>();
px->SetFactorX(0.06F);
px->SetCameraReference(cameraGo);
```

#### C\#

```csharp
bg.AddSprite(skyKey, tint, uv, sortOrder: 5);
bg.GetOrAddComponent<ParallaxLayerComponent>()?.SetFactorX(0.06f);
```

#### Python

```python
bg.add_sprite(sky_key, tint, uv, sort_order=5)
px = bg.get_or_add_default_component(ComponentKind.ParallaxLayer)
```

---

## Scene2DCompositeViewComponent

**Header:** `spark/ecs/components/rendering/Scene2DCompositeViewComponent.hpp`  
Describes orthographic **offscreen captures** (minimap, fog-of-war). Triggers `VulkanScene2DCompositeCapture` after the main pass.

**When to use:** Minimap UI, fog reveal in grid ARPG (`GameFlow2DProductPathDemo`).

#### C++

```cpp
#include "spark/ecs/components/rendering/Scene2DCompositeViewComponent.hpp"

auto* view = mapRoot->AddComponent<Scene2DCompositeViewComponent>();
view->SetViewKind(Scene2DCompositeViewKind::Minimap);
view->SetWorldHalfExtents({24.0F, 14.0F});
```

#### C\#

```csharp
mapRoot.GetOrAddComponent<Scene2DCompositeViewComponent>()?.SetViewKind(Scene2DCompositeViewKind.Minimap);
```

#### Python

```python
map_root.get_or_add_default_component(ComponentKind.Scene2DCompositeView)
```

---

## ParticleEmitterComponent (sprite layer)

**Header:** `spark/ecs/components/rendering/ParticleEmitterComponent.hpp`  
CPU billboard particles; set `ParticleRenderSpace::SpriteLayer` to sort with sprites.

#### C++

```cpp
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/scene/vfx/VfxLibrary.hpp"

auto* fx = world.CreateGameObject();
fx->AddComponent<TransformComponent>()->SetTranslation({x, y, 0.1F});
ParticleEmitterComponent* pe = fx->AddComponent<ParticleEmitterComponent>();
VfxLibrary::ApplyBuiltin(VfxBuiltinId::CoinPop2D, *pe);
pe->SetRenderSpace(ParticleRenderSpace::SpriteLayer);
```

#### C\#

```csharp
var fx = world.CreateGameObject("Fx");
fx.GetOrAddTransform().SetTranslation(new Vector3(x, y, 0.1f));
var pe = fx.GetOrAddComponent<ParticleEmitterComponent>();
VfxLibrary.ApplyBuiltin(VfxBuiltinId.CoinPop2D, pe);
pe.SetRenderSpace(ParticleRenderSpace.SpriteLayer);
```

#### Python

```python
fx = world.create_game_object("Fx")
fx.get_or_add_transform().set_translation(Vector3(x, y, 0.1))
pe = fx.get_or_add_default_component(ComponentKind.ParticleEmitter)
```

---

## VfxPlayerComponent

**Header:** `spark/ecs/components/rendering/VfxPlayerComponent.hpp`  
Plays `.sparkvfx` assets or builtin keys; pairs with transform for spawn origin.

#### C++

```cpp
#include "spark/ecs/components/rendering/VfxPlayerComponent.hpp"

auto* player = fxGo->AddComponent<VfxPlayerComponent>();
player->SetVfxAssetKey("footstep_puff_2d");
player->PlayOnce(*fxGo);
```

#### C\#

```csharp
fxGo.GetOrAddComponent<VfxPlayerComponent>()?.SetVfxAssetKey("footstep_puff_2d").PlayOnce(fxGo);
```

#### Python

```python
vfx = fx_go.get_or_add_default_component(ComponentKind.VfxPlayer)
```

---

## Submit reminder

Nothing draws until `FillStandardLitSceneFromWorld` / `SubmitStandardLitSceneFromWorld*` runs with a valid `Camera2DComponent` or custom `viewProjection`. Use `SceneSpriteSortMode::SortOrderThenWorldY` for side-view games.

Next: [3D rendering components](02-3d-rendering.md).
