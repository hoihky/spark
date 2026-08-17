# Open-World Action — Engine & Game Feature Roadmap

This document is a **detailed plan** for evolving Spark toward **open-world action** games: large spaces, traversal, combat readability, streaming, and production discipline. It assumes Spark’s current architecture (ECS, `Scene` queries, `SceneRenderParams` → Vulkan forward path, lit meshes, skinning, terrain, sky, particles, UI) as described in `docs/ARCHITECTURE_AND_DEVELOPER_GUIDE.md`. For a focused gap analysis of animation, materials, and scene management for 3D action games, see [`3D_ACTION_GAME_GAPS.md`](3D_ACTION_GAME_GAPS.md).

Use this as a **living spec**: adjust phases when you lock decisions in §0.

---

## 0. Lock the fantasy (decisions that steer everything)

Before heavy feature work, pin these so the engine bets match the game:

1. **World topology** — One continuous map vs stitched regions vs procedural chunks.
2. **Player fantasy** — Grounded action vs parkour vs superhuman traversal.
3. **Combat camera** — Over-the-shoulder lock-on vs free cam vs hybrid.
4. **Online** — Offline-only vs co-op vs live service (drives saves, authority, anti-cheat).
5. **Target platforms & min spec** — Drives streaming granularity and shadow/light budgets.
6. **Day/night + weather** — Yes/no; drives lighting, sky, and gameplay readability.
7. **Vehicles** — Yes/no (large cost: physics, animation, AI, streaming).
8. **Interiors** — Rare enterable buildings vs many vs mostly façade.
9. **NPC density** — Crowd tech vs sparse believable agents.
10. **Mission structure** — Golden path + side content vs mostly systemic loops.
11. **Peak load** — Order-of-magnitude: max drawables, physics bodies, AI agents active at once.
12. **Content pipeline** — DCC (Blender/Maya) + glTF rules; skeleton limits; texture formats.

---

## 1. North-star technical pillars

| Pillar | Why it matters |
|--------|----------------|
| **Streaming & memory** | Open world fails without bounded RAM and stable frame time. |
| **Spatial queries & AI** | Combat, stealth, encounters, navigation need fast “near what” queries. |
| **Traversal + animation** | Action feel is largely locomotion, hits, and IK. |
| **Combat loop** | Damage, i-frames, staggers, weapons—not only raw colliders. |
| **Rendering scale** | LOD, shadows, time-of-day; readable combat at distance. |
| **Missions & persistence** | Quests and world flags must survive streaming and saves. |
| **Profiling & tooling** | Open worlds are not shippable without repeatable perf and content validation. |

---

## 2. Phased plan (dependency order)

Each phase **unlocks** the next. Within a phase: **P0** (must) → **P1** → **P2** (nice).

### Phase A — Foundations (world model + budgets)

**Goal:** Load a *region* without loading the whole map; measure it reliably.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| A1 | **World partitioning contract** — region/cell/layer IDs, coordinate spaces, streaming roots | P0 | Data-driven region descriptors: bounds, dependencies, preload hints |
| A2 | **Frame & memory budgets** — caps on draws, skinned draws, particles, UI verts, physics substeps | P0 | Enforce in submit + sim paths |
| A3 | **Memory pools** for streaming assets (textures, meshes, nav data) | P1 | Reduces fragmentation and spikes |
| A4 | **Determinism hooks** — fixed timestep option, stable ordering where needed | P0 | QA/replay; prerequisite if you ever add netcode |
| A5 | **Seedable RNG service** for encounters | P1 | Reproducible world events |

**Exit criteria:** Load/unload a region in a harness with **stable FPS** and **bounded RAM** on a min-spec profile.

---

### Phase B — Streaming I/O (spine of open world)

**Goal:** Disk → memory → GPU without unacceptable hitching.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| B1 | **Async resource system** — background queue, priorities (near player vs horizon) | P0 | |
| B2 | **Cancellation** when player u-turns; ref-counting for shared assets | P1 | |
| B3 | **Region lifecycle** — `LoadRegion` / `UnloadRegion`, explicit GPU+CPU teardown | P0 | |
| B4 | **Staged region load** — collision/low-res first, visuals after | P1 | Reduces hitches |
| B5 | **Region overlay state** — destroyed props, doors, dropped loot | P0 | Serialize per chunk |
| B6 | **Designer vs player state** conflict rules | P1 | Patches, versioning |

**Exit criteria:** Traverse **three regions** in sequence without hitches beyond agreed budgets (e.g. cap spike ms per frame).

---

### Phase C — LOD & visibility (scale without melting GPU)

**Goal:** Far geometry is cheap; near geometry is rich.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| C1 | **Mesh LOD** — metadata + distance selection in `Scene` or submit path | P0 | |
| C2 | **LOD hysteresis** — reduce popping | P1 | |
| C3 | **Impostors / billboards / simplified clusters** for distant architecture | P1 | |
| C4 | **Frustum + sector culling** — aggressive defaults | P0 | Leverage existing `Scene` spatial options |
| C5 | **GPU occlusion / PVS** | P2 | Only if profiling proves need |

**Exit criteria:** Same route shows **~2× fewer draws** at mid-distance vs naive submit, without unacceptable visual regression.

---

### Phase D — Rendering (“action open world” readability)

**Goal:** Readable combat at noon; mood at dusk/night.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| D1 | **Sun + time of day** — dominant directional from game clock | P0 | Wire to sky where applicable |
| D2 | **Cascaded shadow maps (CSM)** for sun | P0 | Non-negotiable for many AAA melee reads |
| D3 | **Night readability** — tuned ambient / hemisphere fill | P0 | Avoid mushy stealth/combat |
| D4 | **Player torch / flashlight** — strict perf cap | P1 | Optional spotlight in forward pass |
| D5 | **SSAO** | — | **Done** — `VulkanScreenSpaceEffectsPass`, `ssaoEnabled` on `SceneRenderParams` |
| D5b | **Contact shadows** | P2 | Short screen-space sun trace (not implemented) |
| D6 | **Weather** — rain/snow particles + surface wetness/roughness | P1 | Start cheap, not full fluid sim |
| D7 | **Volumetric fog / god rays** | P2 | Separate pass or raymarch (not implemented) |

**Exit criteria:** **Day → night** combat encounter stays **readable** within GPU budget.

---

### Phase E — Physics & queries (scale + trust)

**Goal:** Weapons, interactions, and AI use the same trustworthy query story.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| E1 | **3D query parity** — raycast / overlap for weapons, interaction, perception | P0 | Align with game layers/filters |
| E2 | **Document supported** shapes/constraints for the shipped game | P0 | Capsule player, ragdoll scope, etc. |
| E3 | **Extend in-house physics** *or* **integrate** Jolt/PhysX | P1 | One big decision; avoid doing both |
| E4 | **Physics LOD** — sleep/distant simplified collision | P1 | |

**Exit criteria:** Melee trace + explosion overlap + vault/mantle probe **stable** on streamed collision.

---

### Phase F — Locomotion, animation, camera

**Goal:** Open world is *felt* through movement and camera.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| F1 | **Character motor** — slopes, steps, coyote time, jump buffer | P0 | |
| F2 | **Mantle / vault** — world markers + traces | P1 | |
| F3 | **Locomotion blend tree** — walk/run/strafe | P0 | |
| F4 | **Hit reactions, staggers** — animation events + gameplay states | P1 | |
| F5 | **IK** — foot on slopes; aim IK; look-at (pick 1–2 for ship) | P1 | |
| F6 | **Camera** — collision pull-in, occluder handling | P0 | |
| F7 | **Lock-on** (if melee-heavy) | P1 | Prototype early if core |

**Exit criteria:** Playtest: **“responsive”** on uneven terrain (qualitative + timing metrics).

---

### Phase G — Combat systems (heart of “action”)

**Goal:** Tunable, debuggable combat—not one-off demo code.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| G1 | **Hit model** — traces, active frames, i-frames | P0 | Prefer animation-driven windows |
| G2 | **Damage pipeline** — types, armor, crit, knockback, optional hitstop | P0 | |
| G3 | **Status effects** — stacking rules | P1 | |
| G4 | **AI combat** — aggro, spacing, ranged spread, flee | P0 | |
| G5 | **Group tactics** — pin, alternate attacks | P1 | |

**Exit criteria:** Designers tune a **weapon** with data/schema, minimal code churn.

---

### Phase H — Open-world “life” (systemic layer)

**Goal:** Busy world without simulating every NPC every frame.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| H1 | **Encounter system** — spawn points, cooldowns, biome tags | P0 | |
| H2 | **Escalation / heat** if player lingers | P1 | |
| H3 | **POI schedules** — lightweight routes + time windows | P1 | |
| H4 | **Factions / crime / reputation** | P2 | Only if core fantasy |

**Exit criteria:** 10-minute drive loop shows **variety** without hand-authoring every fight.

---

### Phase I — Missions, narrative, persistence

**Goal:** Leave and return; quests survive streaming.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| I1 | **Quest/objective graph** — anchors to region + entity + fallbacks | P0 | |
| I2 | **Branching** with save-safe flags | P1 | |
| I3 | **Chunked saves** aligned to regions | P0 | |
| I4 | **Save versioning / migration** | P1 | |

**Exit criteria:** Save mid-quest → stream out → return → **state intact**.

---

### Phase J — Tools & production

**Goal:** Content ships without engine dev in the daily loop.

| ID | Feature | Priority | Notes |
|----|---------|----------|--------|
| J1 | **Validators** — missing LOD, bad collision, oversized textures, nav gaps | P0 | |
| J2 | **Live tuning** — combat/spawn constants (e.g. ImGui) | P1 | |
| J3 | **Automated perf routes** — scripted flythrough, nightly timings | P0 | |

**Exit criteria:** Vertical slice owned primarily by **design + art** with engine support as needed.

---

## 3. Vertical slice milestones (ordered proof builds)

| # | Slice | Proves |
|---|--------|--------|
| 1 | **Streamed strip** | 3 regions, dummy combat, day/night, perf HUD |
| 2 | **Town encounter** | Enemies + mini-boss + objective in streamed area |
| 3 | **Side loop** | Collectible + vendor + save/load after unload |
| 4 | **“E3 vertical”** | One polished biome + weather + shadows at sunset + crowd-lite |

---

## 4. Risk register

| Risk | Mitigation |
|------|------------|
| Streaming hitches | Async IO, staged GPU upload, strict budgets |
| LOD pop | Hysteresis, dither/fade, art rules |
| Unreadable combat (no shadows) | Prioritize CSM (Phase D) early |
| Physics blow-ups at scale | Region sleep, simplified far collision, caps |
| Save corruption | Atomic chunk writes, checksums, versioned schema |
| Scope creep (vehicles, bases, …) | Gate on §0 decisions |

---

## 5. Mapping to Spark today (honest anchors)

- **Use:** ECS + `Scene` + `SceneRenderParams` snapshot model; existing lit path and skinning; terrain/sky/particles/UI.
- **Expect large engine bets:** **streaming**, **shadows + scaled lighting**, and **physics/query breadth** are typical long poles for open-world action.
- **Defer until proven:** full GI, full fluid/water sim, city-scale crowds — each is a major program on its own.

---

## 6. Example 18-month critical path (small engine team)

| Quarter | Focus |
|---------|--------|
| **Q1** | Phases A–B, D1–D2 (sun + CSM prototype), perf harness |
| **Q2** | Phase C, E1, Slice 1 |
| **Q3** | Phases F–G, Slice 2 |
| **Q4** | Phases H–I, Slice 3 |
| **Q5–Q6** | D3–D4 polish, Phase J, Slice 4, scope cut to ship |

Adjust length if team size or scope changes.

---

## 7. References in this repo

- `docs/ANIMATION_3D_ROADMAP.md` — tracked 3D animation milestones (M1–M6); maps to Phase F/G items below.
- `docs/GUI_EDITOR_ROADMAP.md` — retained GUI inventory + Spark Editor milestones (E0–E6).
- `docs/ARCHITECTURE_AND_DEVELOPER_GUIDE.md` — ECS, rendering, physics summary, `SceneRenderParams`, Vulkan overview.
- `include/spark/engine/SceneRenderParams.hpp` — GPU snapshot contract.
- `include/spark/scene/Scene.hpp` — world queries and optional spatial policies.
- `include/spark/physics/PhysicsWorld3D.hpp` — 3D simulation entry point.
- 3D demos under `include/spark/demo/` / `src/spark/demo/` (e.g. `ThreeDDemo`, `Maze3DDemo`, `PhysicsBallThrow3DDemo`, `SceneEditor3DDemo`).

---

*Last updated: written for Spark open-world action planning; revise as product scope locks.*
