# Engine C++ API — Gap Analysis (Scene & 3D Rendering)

Gap analysis for Spark’s **public C++ API** under `include/spark/`. This document is for **gameplay and engine contributors** building with `IGame`, `GameWorld`, `Scene`, and `SceneRenderParams` — **not** editor tooling (`spark/editor/` is out of scope).

Complements [`ARCHITECTURE_AND_DEVELOPER_GUIDE.md`](ARCHITECTURE_AND_DEVELOPER_GUIDE.md) (feature catalog) and material/lighting notes in [`MATERIALS_AND_LIGHTING.md`](MATERIALS_AND_LIGHTING.md).

---

## 1. API surface today

Gameplay code should depend on these headers, not on `VulkanRenderer` internals.

### Application & frame contract

| API | Role |
|-----|------|
| `IGame` | `OnAttach` / `OnUpdate` / `OnRender` / `OnDetach` |
| `Game` | Optional base owning `Scene`; forwards `UpdateGameObjects` |
| `IEngineContext` | `GetInput`, `GetFramebufferSize`, `SetSceneRenderParams`, `TryGetSoundEngine`, `TryGetScene` |
| `IFramePresenter` | Abstract present; default impl is Vulkan (games rarely include this header) |
| `FrameTiming` | `deltaTimeSeconds`, `totalTimeSeconds`, `frameIndex` |
| `SceneRenderParams` | Per-frame GPU snapshot: draws, lights, sprites, particles, UI, textures |

### World & ECS

| API | Role |
|-----|------|
| `GameWorld` | `CreateGameObject`, `DestroyGameObject`, `SetParent`, `UpdateGameObjects`, asset `Load*` / `Register*` caches |
| `GameObject` | `AddComponent<T>`, `GetComponent<T>`, `GetWorldMatrix`, signals |
| `GameComponent` | `OnAttach` / `OnUpdate` / `OnDetach` / `OnSignal`; **32** `ComponentKind` values |
| `Scene` | Query iterators: `ForEachDrawable`, `ForEachSkinnedDrawable`, lights, sky, GUI, particles; frustum variants |

### Scene → render bridge

| API | Role |
|-----|------|
| `FillStandardLitSceneFromWorld` | Walk ECS → fill `SceneRenderParams` (optional `Scene*` for frustum culling) |
| `SubmitStandardLitSceneFromWorld` | Fill + `IEngineContext::SetSceneRenderParams` |
| `ApplyMaterialComponentToSceneDrawItem` | Map `MaterialComponent` textures/scalars onto `SceneDrawItem` |
| `PaintGuiCanvases` / `ProcessGuiCanvasesInput` | Retained GUI → params + input |

### Persistence

| API | Role |
|-----|------|
| `SceneSerializer` / `SceneDeserializer` | Text format **`spark_scene_v4`** (reads v3) |
| `SceneDocument` / `SceneDocumentHeader` | Entity list + optional `name`, `assets_root`, `scene_uid` headers |
| `ComponentSnapshotRegistry` | Pluggable handlers; **18** kinds registered today |
| `SceneManager` | Runtime load/unload of scene files on one `GameWorld` (additive by default) |
| `GameWorldAssetLoader` | Background decode for glTF, skinned glTF, textures, OBJ; commit via `Pump` |

**v4 file layout:**

```
spark_scene_v4
<entity_count>
H name "My Level"
H assets_root "assets"
H scene_uid <uint64>
E <id> <parentId> "Name"
C <kind> <payload>
...
```

### Assets & spatial

| API | Role |
|-----|------|
| `Mesh`, `SkinnedMesh`, `Skeleton`, `Texture2D`, `Font` | CPU-side assets |
| `GameWorld::LoadMesh`, `LoadGltf`, `LoadSkinnedGltf`, `LoadTexture` | Path-keyed synchronous load |
| `ScenePartitionKind`, `SceneSpatialPolicyComponent` | Frustum culling mode (BVH, octree, …) |
| `MeshRaycast` | CPU triangle ray tests |

### Cameras & lighting (C++ helpers)

| API | Role |
|-----|------|
| `Camera`, `Camera2D`, `CameraComponent` | View/projection |
| `FlyCamera`, `CharacterCameraRig` | Demo-grade controllers (not ECS components) |
| `PointLightComponent`, `SpotLightComponent`, `DirectionalLightComponent` | Punctual + directional lights → `SceneRenderParams` |
| `SceneLightingProfile` | Outdoor / interior presets for sun, shadows, exposure |
| Directional sun | `DirectionalLightComponent` overrides submit params when present; explicit `SceneRenderParams` fields remain fallback |

### Physics, animation, audio (adjacent APIs)

| API | Role |
|-----|------|
| `SimulatePhysics2D` / `SimulatePhysics3D` | Minimal solvers; call from `OnUpdate` |
| `PhysicsQueries2D` | Overlap / raycast helpers |
| `AnimatorComponent`, `Skeleton`, `AnimLoopMode` | Skeletal playback, crossfade, joint palette |
| `SoundEngine`, `SoundCueComponent` | Playback stack |
| `SimulateGameAi` | Optional AI tick |

---

## 2. Scene management API gaps

**P0** = blocks shipping most games with only the public API; **P1** = scale/quality; **P2** = polish.

### 2.1 `GameWorld` / `GameObject` / `Scene`

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **Stable handles** | `GameObject*` only | `EntityId` or opaque handle surviving reorder; no dangling-pointer-safe lookup |
| **Multi-scene** | `SceneManager` additive load/unload on one `GameWorld` | Named scene registry, cross-scene references, persistent “manager” world separate from gameplay |
| **Prefab API** | Manual duplicate in code | `InstantiatePrefab(path, parent, overrides)` with component override map |
| **Tags / layers** | Name string on `GameObject` | `Tag`, `LayerMask`, query by tag/layer on `Scene` |
| **Deferred destroy** | `DestroyGameObject` immediate | `DestroyAtEndOfFrame` to avoid iterator invalidation during `OnUpdate` |
| **Component queries** | Per-object `GetComponent<T>` | World queries: `ForEach<MeshComponent>`, archetype-style iteration |
| **Enable/disable** | Per-component visibility flags vary | Uniform `GameObject::SetEnabled` affecting update + render submit |

### 2.2 `ComponentSnapshotRegistry` / serialization

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **Handler coverage** | 19 / 32 kinds | Handlers for `GuiCanvas`, `Tilemap`, 2D physics, `AiAgent`, … |
| **Asset references** | Paths embedded in snapshots | Stable asset IDs, optional binary format, version migration hooks |
| **Partial apply** | `Apply` creates full document | Merge into existing world, diff/patch documents |
| **Runtime registration** | `ComponentSnapshotRegistry::Default()` fixed set | Public `RegisterHandler` for game-specific `GameComponent` subclasses |

Registered today: Transform, Mesh, Material, DirectionalLight, PointLight, SpotLight, Camera, SkinnedMesh, Animator, Sky, Sprite, SceneSpatialPolicy, TextOverlay, ParticleEmitter, Terrain, BoxCollider3D, SphereCollider3D, Rigidbody3D, PhysicsMaterial3D.

**Runtime load flow:** `SceneManager::BeginLoadSceneAsync` → entities created immediately → asset-dependent components deferred → call `Pump()` each frame until `IsSceneReady`. Sync path: `LoadSceneFromFile` pumps until ready.

### 2.3 Asset loading (`GameWorld`)

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **Async load** | `GameWorldAssetLoader` + `SceneManager::Pump` | Per-asset callbacks, priority queues, cancellation |
| **No unload** | Cache grows until world destroyed | `ReleaseMesh(path)`, ref-counted `AssetHandle` |
| **glTF surface** | Rigid + skinned mesh paths | Morph targets, multiple UV sets, animation events, sparse accessors |
| **Material import** | Textures via separate `MaterialComponent` setup | `LoadGltfMaterial` or auto-apply from glTF to `MaterialComponent` |
| **Streaming hooks** | None | `IAssetStreamer` interface for region-based load/unload |

### 2.4 `Scene` query & culling API

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **Occlusion** | Frustum culling only (`ForEach*InViewFrustum`) | Occlusion query API or prebuilt cell visibility |
| **LOD** | Always full mesh | `MeshLodComponent` + `Scene::ForEachDrawableLod(view, distance)` |
| **Light iterators** | Point, spot, directional (`ForEachDirectionalLight`) | Area lights, light probes |
| **Physics overlap from Scene** | Separate `PhysicsQueries2D` | `Scene::OverlapSphere`, `Raycast` unified with 3D physics bodies |
| **Picking** | `Scene::RaycastPick` (rigid + skinned bind pose); `MeshRaycast` for low-level use | Physics-body picks, deformed skinned mesh picks |

### 2.5 Physics API (`spark/physics/`)

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **3D scope** | Spheres vs static boxes + sphere–sphere | Capsules, meshes, convex hulls, compound colliders |
| **Character controller** | None | `CharacterController3D::Move(intent)` on public API |
| **Triggers** | 2D trigger signals partial | `OnTriggerEnter3D` component callbacks |
| **Fixed timestep** | `PhysicsWorld3DSettings::substeps` only | `IPhysicsWorld::Step(fixedDt)` decoupled from render rate |
| **Determinism** | Float integration | Documented deterministic mode or fixed-point option |

### 2.6 Animation API (`spark/animation/`, `AnimatorComponent`)

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **Root motion** | Joint palette only | `AnimatorComponent::ConsumeRootMotion()` |
| **Animation events** | None | `OnAnimationEvent` / clip notify markers |
| **Blend trees** | Single clip + crossfade | `BlendTree`, 1D/2D parameter blend |
| **IK** | None | Foot IK, look-at IK hooks on `Skeleton` |
| **State machine** | `Character3DAnimFsmComponent` (locomotion) | General `AnimationStateMachineComponent` API |

See [`ANIMATION_3D_ROADMAP.md`](ANIMATION_3D_ROADMAP.md) for planned milestones.

### 2.7 Gameplay layer

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **`spark/gameplay/`** | Empty directory | No public damage, health, inventory, interaction interfaces — games roll their own |

---

## 3. Rendering API gaps (`SceneRenderParams` & submit path)

Games control rendering through **`SceneRenderParams`** and submit helpers — not through Vulkan types.

### 3.1 `SceneDrawItem` / submit model

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **Transparency queue** | `transparentDraws` + `SceneTransparentSortMode` (material opacity &lt; 1); opaque `draws` | `SceneDrawItem::sortKey` for fine-grained ordering within queues |
| **Render layers** | `shadowFlags` only | `layerMask`, `renderQueue` (background / opaque / transparent / overlay) |
| **Instancing** | One `SceneDrawItem` per object | `InstanceBatch` or `drawInstances` with shared mesh + per-instance data |
| **LOD selection** | Caller picks mesh | Submit helper chooses LOD from camera distance |
| **Custom shaders** | `SceneShadingModel::LitPbr` / `ToonCel` | Material shader graph or `userShaderId` escape hatch |

### 3.2 `SceneRenderParams` limits

| Field / cap | Value | Gap |
|-------------|-------|-----|
| `sceneTextures` | 16 RGBA8 layers | No bindless handle; games with many unique materials must atlas or batch |
| `MaxPointLights` | 256 | OK for forward; no API to prioritize / cull lights per tile |
| `MaxSpotLights` | 128 | Same |
| Punctual shadows | Max 2 point + 4 spot per frame | No API to assign shadow priority to lights |
| `MaxSprites` | Fixed cap on `sprites` array | Documented limit; no spill or batch API |
| Skinning | 64 joints per draw | No multi-draw skinned instancing |
| Custom meshes | `SceneMeshSlot::Custom` + per-frame GPU upload | No persistent `GpuMeshHandle` owned by game code |

### 3.3 Materials (`MaterialComponent` → `SceneDrawItem`)

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **glTF extensions** | Core PBR + emissive map | Clearcoat, sheen, transmission, unlit |
| **Second UV** | Single UV in mesh | `uv1` channel on `Mesh` / material |
| **Decals** | None | `DecalProjectorComponent` → decal draw list |
| **Directional light ECS** | `DirectionalLightComponent` + `FillStandardLitSceneFromWorld` override | Multiple directional lights with blending / priority |
| **Reflection probes** | `iblEnabled` + env layer index | `ReflectionProbeComponent`, blend weights per object |
| **Material instances** | Per-object `MaterialComponent` | Shared `MaterialAsset` + instance overrides |

### 3.4 Lighting & atmosphere (params-level)

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **Post stack** | `ssaoEnabled` + tonemap exposure | `bloomStrength`, `vignette`, color grading LUT slot on params |
| **Fog** | None | `fogColor`, `fogDensity` / height fog on `SceneRenderParams` |
| **Contact shadows** | None | Toggle + tunables on params |
| **Time of day** | `useTimeOfDay`, `timeOfDay` | Documented; no `TimeOfDaySystem` component driving params |
| **Area lights** | None | `RectLightComponent` |

Details: [`LIGHTING_AND_SHADOWS.md`](LIGHTING_AND_SHADOWS.md), [`MATERIALS_AND_LIGHTING.md`](MATERIALS_AND_LIGHTING.md).

### 3.5 Cameras & viewports (params-level)

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **Single view** | One `viewProjection` on params | `SceneRenderParams` array for split-screen / picture-in-picture |
| **Offscreen targets** | Swapchain + full-frame HDR only | `IRenderTarget` / `RenderTexture` handle for mirrors, UI 3D, thumbnails |
| **Viewport rect** | `worldViewportScissor*` (clip rect) | Sub-rect render without scissor hack; normalized viewport on params |
| **Orthographic 3D** | `Camera` supports it | First-class in `SubmitStandardLitSceneFromWorld` + shadow fitting |

### 3.6 2D / UI render API (runtime)

| Gap | Current API | Missing API / behavior |
|-----|-------------|------------------------|
| **Sprite batching** | Per-sprite `SceneSpriteDraw` | `SubmitSpritesBatched` from `SpriteComponent` tiles |
| **GUI paint order** | `paintOrder` on draws; GPU pass batches rects then text per layer | Documented limitation; no `ForceInterleavedUiPaint` flag |
| **World-space UI** | Screen-space `GuiCanvasComponent` only | World-space canvas or `BillboardWidget` component |

---

## 4. Cross-cutting API design gaps

| Gap | Impact |
|-----|--------|
| **No render abstraction enum** | Games include `SceneRenderParams` Vulkan-specific comments (clip Y, shadow flip) |
| **No job system** | All load, cull, submit on main thread |
| **No event bus** | `EmitSignal` is per-object only; no world-level `Subscribe<CollisionEvent>` |
| **C++ scripting surface** | `SparkInterop.h` C ABI exists; not all `ComponentKind` values exposed to C# |
| **Error reporting** | `LoadMesh` returns empty `SharedPtr`; no `LoadResult` with error code / message |
| **Thread safety** | `GameWorld` is main-thread only; not documented on public headers |

---

## 5. Recommended API evolution order

Prioritized for **C++ game authors** (no editor dependency):

| Phase | Public API work | Unlocks |
|-------|-----------------|---------|
| **1** | Serialization handlers for lights, sprites, physics, terrain | Save/load real gameplay scenes |
| **2** | ~~`DirectionalLightComponent`; `Scene::RaycastPick`~~ | Less boilerplate in `OnRender` |
| **3** | ~~Transparent draw list + sort on `SceneRenderParams`~~ | Glass, foliage, alpha meshes |
| **4** | `AssetHandle` + async `RequestLoad*` | Large levels without hitches |
| **5** | `EntityId` + deferred destroy | Safer gameplay code |
| **6** | Instancing fields on submit path | Crowds, props |
| **7** | `RenderTexture` / multi-view `SceneRenderParams` | Mirrors, split-screen |
| **8** | Character controller + trigger callbacks (3D) | Action games without custom physics |

---

## 6. Strengths to preserve

- **`IGame` + `IEngineContext`** — small, stable gameplay boundary.
- **`SceneRenderParams` snapshot** — simulation and GPU stay decoupled; easy to test fill logic.
- **`Scene` iterators + `SceneSubmit`** — opt-in manual path or one-call submit.
- **`ComponentSnapshotRegistry`** — extend serialization without changing `SceneSerializer` signature.
- **Header modularization** — `spark/ecs/components/*.hpp`, `spark/scene/`, `spark/engine/` compose without monolithic include.

---

## 7. Related documents

| Doc | Focus |
|-----|-------|
| [`ARCHITECTURE_AND_DEVELOPER_GUIDE.md`](ARCHITECTURE_AND_DEVELOPER_GUIDE.md) | Full API catalog and frame flow |
| [`MATERIALS_AND_LIGHTING.md`](MATERIALS_AND_LIGHTING.md) | Material channels and shader limits |
| [`LIGHTING_AND_SHADOWS.md`](LIGHTING_AND_SHADOWING.md) | Shadow/light implementation status |
| [`ANIMATION_3D_ROADMAP.md`](ANIMATION_3D_ROADMAP.md) | Animation API roadmap |
| [`OPEN_WORLD_ACTION_ROADMAP.md`](OPEN_WORLD_ACTION_ROADMAP.md) | Long-horizon streaming/combat (many items need new public API) |
| [`CSHARP_SCRIPTING.md`](CSHARP_SCRIPTING.md) | Managed interop surface |

---

*Last updated: 2026-07 — 32 `ComponentKind` values, 19 serialization handlers, `DirectionalLightComponent`, `Scene::RaycastPick`, `transparentDraws` submit + Vulkan pass.*
