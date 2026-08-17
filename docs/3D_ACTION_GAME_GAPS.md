# 3D Action Game — Gap Analysis

Gap analysis across Spark's **animation**, **texture/material**, and **scene management** systems for shipping a **3D action game** (combat arenas, locomotion, weapons, level flow, multiple enemies).

Complements:

- [`ANIMATION_3D_ROADMAP.md`](ANIMATION_3D_ROADMAP.md) — animation milestone tasks
- [`MATERIALS_AND_LIGHTING.md`](MATERIALS_AND_LIGHTING.md) — PBR pipeline and limits
- [`SCENE_AND_RENDERING_GAPS.md`](SCENE_AND_RENDERING_GAPS.md) — public C++ API gaps
- [`OPEN_WORLD_ACTION_ROADMAP.md`](OPEN_WORLD_ACTION_ROADMAP.md) — long-horizon streaming/combat/AI

---

## Executive summary

Spark has a **solid prototype foundation**: skeletal playback, forward PBR, additive scene loading, and `spark_scene_v4` serialization. It is **not yet production-ready** for a shipping action title.

| System | Maturity | Action-game verdict |
|--------|----------|---------------------|
| **Animation** | M1 done, M2 partial | Locomotion works; combat depth (blend trees, hurt/stagger, root motion, IK, events) is missing |
| **Textures / materials** | PBR pipeline exists | Mipmapped scene arrays; BC7/ASTC (KTX2 + runtime BC7); 32×1024² budget |
| **Scene management** | Core load/save works | No prefabs, 3D spawn points, transitions, physics layers, or streaming |

---

## 1. Animation system

### What exists

| Capability | Evidence |
|------------|----------|
| glTF skinned import, GPU skinning (≤64 joints) | `src/spark/scene/skinned_mesh_gltf.cpp`, `shaders/scene.vert` |
| `AnimatorComponent` — loop modes, crossfade, clip API | `include/spark/ecs/components/animation/AnimatorComponent.hpp`, `include/spark/animation/Skeleton.hpp` |
| `Character3DAnimFsmComponent` — idle/walk/run + attack one-shot | `include/spark/ecs/components/animation/Character3DAnimFsmComponent.hpp` |
| Manual animation events → signals | `include/spark/ecs/components/animation/AnimationEventReceiverComponent.hpp` |
| Bone attachments by joint index | `include/spark/ecs/components/animation/AttachmentSocketComponent.hpp` |
| C# bindings for animator + 3D FSM | `include/spark/scripting/SparkInterop.h`, `scripting/bindings/generated/Spark.Bindings/ComponentMirrors.g.cs` |
| Scene save: skinned mesh + animator | `src/spark/scene/serialization/ComponentSnapshotRegistry.cpp` |

### Gaps for 3D action

| Priority | Gap | Why it matters |
|----------|-----|----------------|
| **P0** | **No speed-based blend tree** (walk↔run) | Discrete clip switches cause pops; action games need smooth locomotion |
| **P0** | **No animation layers** (upper-body combat while moving) | Cannot hold weapon aim / attack overlay on lower-body locomotion |
| **P0** | **Combat FSM is thin** — attack only; no hurt/stagger/death/block | 2D FSM (`Sprite2DCharacterAnimFsmComponent`) is richer; 3D has no parity |
| **P0** | **No root motion** | Melee dashes, dodge rolls, attack lunge need motion from clips |
| **P1** | **No glTF animation events** | Hit windows, footstep SFX, VFX spawn must be hand-coded markers |
| **P1** | **No foot IK / aim IK** | Foot sliding on slopes; aim offset while strafing |
| **P1** | **Joint lookup by name** | Weapon sockets require raw `jointIndex`; no `"RightHand"` API |
| **P1** | **FSM / events / sockets not serialized** | Save/load loses combat setup |
| **P2** | **64-joint cap, first skinned node only** | Complex rigs or multi-mesh characters fail |
| **P2** | **No morph targets** | Facial hit reactions, damage states |
| **P2** | **No palette cache / skinned draw budget** | Many NPCs will be expensive |

### Action-game readiness (animation)

| Need | Status | Notes |
|------|--------|-------|
| Load animated humanoid from glTF | **Exists** | Fox/CesiumMan demos; ≤64 joints |
| Idle / walk / run locomotion | **Exists** | Discrete clips + crossfade; not blended by speed |
| Melee attack overlay | **Partial** | `RequestAttack()` + Once clip; no hurt/stagger |
| Combo / cancel system | **Missing** | No state graph, no interrupt rules |
| Hitbox spawn on anim frame | **Partial** | Manual `AnimationEventReceiverComponent` markers; no glTF pipeline |
| Weapon attach to hand bone | **Partial** | `AttachmentSocketComponent` by index; no name lookup |
| Root-motion attacks | **Missing** | |
| Foot sliding fix (foot IK) | **Missing** | |
| Save/load animated enemy | **Partial** | Mesh + animator only; FSM/events/sockets lost |

**Roadmap:** [`ANIMATION_3D_ROADMAP.md`](ANIMATION_3D_ROADMAP.md) — M2 partial, M3–M6 open.

---

## 2. Texture mapping & material system

### What exists

| Capability | Evidence |
|------------|----------|
| Full PBR material model (albedo, normal, ORM, emissive, AO) | `include/spark/ecs/components/rendering/MaterialComponent.hpp`, `shaders/scene.frag` |
| Per-draw texture layers via push constants + 32-layer array | `include/spark/render/scene/VulkanSceneDescriptors.hpp`, `src/spark/scene/SceneSubmitMaterial.cpp` |
| glTF PBR material import (base color, normal, ORM, emissive + factors) | `include/spark/scene/GltfMaterial.hpp`, `src/spark/scene/gltf_material.cpp` |
| Mipmapped scene texture array (GPU blit or uploaded mips) | `src/spark/render/scene/VulkanSceneTextureUploader.cpp` |
| BC7 runtime encode + ASTC via KTX2 (`Texture2D::TryLoadFromKtx2File`) | `src/spark/scene/texture_block_compression.cpp`, `src/spark/scene/texture_ktx2.cpp` |
| glTF mesh UV import (`TEXCOORD_0`) | `src/spark/scene/mesh_gltf.cpp`, `include/spark/scene/SkinnedMesh.hpp` |
| Sprite/tilemap atlas UV (2D) | `include/spark/ecs/components/rendering/SpriteComponent.hpp`, `src/spark/scene/SceneTileAtlas.cpp` |
| Async texture decode | `include/spark/scene/GameWorldAssetLoader.hpp` |
| IBL, SSAO, shadows, clustered lights | `shaders/ibl.glsl`, post passes |
| Material showcase demo | `src/spark/demo/MaterialShowcase3DDemo.cpp` |

### Architecture constraint

```text
MaterialComponent → SceneDrawItem (up to 4 layers/draw)
                 → sceneTextures[32] @ 1024², full mip chain
                 → BC7 / ASTC 4x4 (device + build) or RGBA8 + GPU mip blit
```

See `include/spark/engine/SceneRenderParams.hpp` (`MaxSceneTextures = 32`) and `include/spark/render/scene/VulkanSceneTextureUploader.hpp`.

### Gaps for 3D action

| Priority | Gap | Why it matters |
|----------|-----|----------------|
| **P1** | **32 textures/frame at 1024²** | Large scenes still overflow the layer budget |
| **P1** | **Runtime ASTC encode needs astc-encoder fetch** | Use `.ktx2` pre-bakes or BC7/RGBA8 fallback until CMake downloads succeed |
| **P1** | **No shared `MaterialAsset`** | Per-instance duplication; no material library |
| **P1** | **Multi-material glTF meshes** | Only primary material per file is imported |
| **P2** | **No GPU decals** | Bullet holes, blood, ability marks need mesh/sprite workarounds |
| **P2** | **Single UV set** | No lightmaps or detail UV |
| **P2** | **No texture streaming** | Large arenas / open hubs |

### Workarounds today

1. Pack environment textures into atlases manually; reuse materials across props.
2. After `LoadGltf` / `LoadSkinnedGltf`, materials are applied automatically via `GltfMaterialDesc` + `ApplyGltfMaterialDesc`.
3. Ship hero props as `.ktx2` (BC7/ASTC mips) for best quality; PNG/JPEG are mipmapped and BC7-encoded at upload when supported.
4. Keep unique textured draws ≤ 32 visible per frame (`MaxSceneTextures`).

---

## 3. Scene management system

### What exists

| Capability | Evidence |
|------------|----------|
| `GameWorld` + hierarchy, parenting, active flag | `include/spark/scene/GameWorld.hpp`, `include/spark/ecs/GameObject.hpp` |
| `SceneManager` — additive/replace load, async assets | `include/spark/scene/SceneManager.hpp`, `include/spark/scene/GameWorldAssetLoader.hpp` |
| `spark_scene_v4` text serialization | `include/spark/scene/serialization/SceneDocument.hpp`, `SceneSerializer.hpp` |
| ~28 component snapshot handlers | `src/spark/scene/serialization/ComponentSnapshotRegistry.cpp` |
| Frustum spatial culling (BVH/octree/etc.) | `include/spark/scene/ScenePartitionKind.hpp`, `src/spark/scene/SceneSpatialCull.cpp` |
| 3D trigger volumes | `include/spark/ecs/components/physics3d/TriggerVolume3DComponent.hpp` |
| Editor prototype save/load | `src/spark/demo/SceneEditor3DDemo.cpp` |
| Regional fog/post volumes | `include/spark/scene/RenderVolumes.hpp` |

### Gaps for 3D action

| Priority | Gap | Why it matters |
|----------|-----|----------------|
| **P0** | **No prefabs** (`InstantiatePrefab`) | Enemies, doors, pickups cannot be reused as authored chunks |
| **P0** | **No 3D spawn points / checkpoints** | 2D has tilemap markers; 3D has no `SpawnPoint3DComponent` |
| **P0** | **No scene transition API** | Hub → arena → respawn is DIY on top of `SceneManager` |
| **P0** | **No physics layer masks** | Player vs enemy vs projectile filtering is tag-on-triggers only |
| **P1** | **Tags not serialized** | `FindGameObjectWithTag("PlayerSpawn")` breaks after save/load |
| **P1** | **Single global physics world** | Unload removes colliders; no per-scene isolation |
| **P1** | **No loading screen / progress API** | Async load exists (`Pump`) but no % or callbacks |
| **P1** | **No persistent player pattern** | One `GameWorld`; hub player + additive arena needs custom architecture |
| **P2** | **No world streaming / regions** | [`OPEN_WORLD_ACTION_ROADMAP.md`](OPEN_WORLD_ACTION_ROADMAP.md) — not implemented |
| **P2** | **Lights are global** | Additive arena lights affect hub; first directional wins |
| **P2** | **~28/71 components serialize** | AI, audio, UI canvas, 2D physics, animation sockets missing |

### Typical action flow today

```text
DIY LevelManager
  → UnloadScene(oldArenaId)
  → BeginLoadSceneAsync(arenaPath, { additive: false })
  → Pump() each frame + status TextOverlay
  → manually FindGameObject / spawn player in code
```

### Comparison matrix

| Feature | Status |
|---------|--------|
| Level as serialized file | ✅ EXISTS |
| Async load (no main-thread hitch on I/O) | ✅ EXISTS |
| Additive hub + overlay UI scene | ⚠️ PARTIAL |
| Replace arena (single active level) | ⚠️ PARTIAL |
| Hub world ↔ combat arena flow | ❌ MISSING |
| Loading screen | ❌ MISSING |
| Player spawn / checkpoint (3D) | ❌ MISSING |
| Prefabs (enemies, doors, pickups) | ❌ MISSING |
| Combat trigger volumes | ⚠️ PARTIAL |
| Physics layers (player vs enemy vs projectile) | ❌ MISSING |
| Streaming open world | ❌ MISSING |
| Frustum cull large levels | ✅ EXISTS |
| Editor save/load production pipeline | ⚠️ PARTIAL |

---

## Cross-system gaps

| Need | Animation | Materials | Scenes | Status |
|------|-----------|-----------|--------|--------|
| Player locomotion + combat | Partial FSM | — | — | Walk/run/attack only |
| Enemy wave spawn | Playback OK | Material budget tight | No prefabs | Manual duplication |
| Hit detection on anim frame | Manual markers only | — | — | No glTF event pipeline |
| Weapon on hand bone | Socket by index | — | — | No joint-by-name |
| Arena load/unload | — | — | Async load works | No transition layer |
| Checkpoint respawn | — | — | No 3D spawn component | Code constants only |
| Save mid-combat | FSM/events lost | PBR maps persist (v2) | Tags lost | Partial round-trip |
| 10+ skinned enemies | No draw budget | 32-texture cap | — | Performance risk |

---

## Prioritized build order

### Tier 1 — Blocks a minimal action prototype

1. **glTF full material import** (normal, ORM, emissive + factors)
2. **Material serialization** — levels round-trip correctly
3. **Prefab instantiate API** — reusable enemy/prop chunks
4. **3D spawn point component** + tag serialization
5. **Speed blend tree** (walk↔run)
6. **Combat FSM expansion** — hurt, stagger, death states

### Tier 2 — Polished combat feel

7. **Animation layers** — upper-body attacks while moving
8. **Root motion** — dodge, lunge, attack displacement
9. **Animation events from glTF** — hit windows, SFX/VFX triggers
10. **Joint-by-name sockets** — weapon attachment
11. **Physics layer masks** — combat collision filtering
12. **Scene transition manager** — load screen, fade, progress
13. **Raise texture budget** — more layers, mips, or bindless path

### Tier 3 — Scale / production

14. Foot IK + aim IK
15. Texture compression + streaming
16. Shared `MaterialAsset` + `.sparkmat` assets
17. World region streaming
18. Skinned draw budget + palette cache
19. Full component serialization (FSM, AI, audio, sockets)
20. GPU decals

---

## What you can ship today vs what you cannot

### Feasible now (with custom game code)

- Single-arena action prototype with 1–2 animated characters
- glTF characters with manual material setup after load
- Level files via `spark_scene_v4` + `SceneManager`
- Ray pick + 3D physics (box/sphere/capsule)
- Attack via `Character3DAnimFsmComponent::RequestAttack()`

### Not feasible without engine work

- Polished locomotion (blend trees, root motion)
- Multi-enemy combat with varied materials in one frame
- Designer-authored hit windows from glTF
- Hub → arena → checkpoint flow without custom `LevelManager`
- Save/load of a full combat scene with materials + FSM + spawn points
- Open-world or streamed sub-regions

---

## Key file index

| Area | Paths |
|------|-------|
| Animation components | `include/spark/ecs/components/animation/` |
| glTF skinned import | `src/spark/scene/skinned_mesh_gltf.cpp` |
| Materials | `include/spark/ecs/components/rendering/MaterialComponent.hpp` |
| Texture upload | `include/spark/render/scene/VulkanSceneTextureUploader.hpp` |
| Scene I/O | `include/spark/scene/serialization/`, `include/spark/scene/SceneManager.hpp` |
| Scene submit | `include/spark/scene/SceneSubmit.hpp` |
| Editor prototype | `include/spark/demo/SceneEditor3DDemo.hpp` |
| Demos | `CharacterCameraDemo.cpp`, `Maze3DDemo.cpp`, `MaterialShowcase3DDemo.cpp` |

---

## Related

- [`ANIMATION_3D_ROADMAP.md`](ANIMATION_3D_ROADMAP.md)
- [`MATERIALS_AND_LIGHTING.md`](MATERIALS_AND_LIGHTING.md)
- [`SCENE_AND_RENDERING_GAPS.md`](SCENE_AND_RENDERING_GAPS.md)
- [`OPEN_WORLD_ACTION_ROADMAP.md`](OPEN_WORLD_ACTION_ROADMAP.md)
- [`2D_ARPG_FEATURES.md`](2D_ARPG_FEATURES.md) — 2D combat FSM reference parity
