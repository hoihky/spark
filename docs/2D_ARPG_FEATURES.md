# 2D ARPG engine gaps — prioritized backlog

Assessment against the current ECS (`GameWorld`, `GameComponent`), 2D physics (`SimulatePhysics2D`), rendering (`SpriteComponent`, `TilemapComponent`, `Camera2D`), AI (`AiAgentComponent`), and GUI.

## P0 — Must-have for a credible ARPG prototype

| Priority | Feature | Status | Notes |
|----------|---------|--------|--------|
| **P0.1** | **Collision layers / filtering** | Implemented | Category + mask bits on `BoxCollider2D` / `CircleCollider2D`; baked into `StaticCollider2D`; honored in dynamic-vs-static resolution. |
| **P0.2** | **Trigger volumes** (overlap without blocking) | Implemented | `GetIsTrigger` / `SetIsTrigger` on 2D colliders; baked into `StaticCollider2D` with `owner`; `SimulatePhysics2D` skips separation for trigger pairs; `SignalId::Physics2DTriggerOverlap` after resolution (`ptr` = other `GameObject*`, `a` = id, `b` = static index). |
| **P0.3** | **World queries** (`OverlapCircle`, `OverlapAabb`, `Raycast` in XY) | Implemented | `PhysicsQueries2D.hpp`: `StaticBroadPhase2D` + `QueryOverlapCircleStatics2D` / `QueryOverlapAabbStatics2D` / `RaycastStatics2D` (same static bake + hash as `SimulatePhysics2D`); convenience `QueryOverlapCircleWorld2D` / `RaycastWorld2D`. Uses `CollisionFilter2D::ShouldCollide`; `hitTriggers` / `hitSolids` flags. **Does not** emit `Physics2DTriggerOverlap` (signals are simulation-only). |
| **P0.4** | **Dynamic-vs-dynamic** resolution | Implemented | After static resolution, dynamic pairs are broad-phased with `SpatialHashGrid2D` (same 4-unit cell as statics), then narrow-tested (same layer rules). Overlapping **triggers** emit `Physics2DTriggerOverlap` (`payload.b` = `kPhysics2DTriggerOverlapNoStaticIndex`). Optional `PhysicsWorld2DSettings::resolveDynamicVsDynamic`: one shallow separation for solid–solid (circle–circle, box–box, box–circle); no second static pass. C: `spark_world_physics_simulate_2d_with_settings`. |

## P1 — Strongly recommended

| Priority | Feature | Status | Notes |
|----------|---------|--------|--------|
| **P1.1** | **Sorting policy for top-down** (Y-sort / painter’s algorithm) | Implemented | <c>SceneSpriteSortMode</c> on <c>SceneRenderParams</c> / <c>SubmitStandardLitSceneFromWorld</c>: <c>SortOrderThenWorldY</c> sorts by ascending <c>sortOrder</c>, then lower world Y on top (+Y-up). C API: last arg to <c>spark_context_submit_standard_lit_scene</c>. |
| **P1.2** | **Character stats / vitals** (`Health`, `Damage`) | Missing | Gameplay layer; either lightweight ECS components + signals or data-only `ActorStats` module (avoid bloating core enum unless stable). |
| **P1.3** | **Animation state machine wiring** | Implemented | <c>Sprite2DCharacterAnimFsmComponent</c> drives <c>SpriteAnimatorComponent</c> clips from <c>Rigidbody2D</c> velocity (idle/move) plus one-shot or <c>AiBlackboard</c> combat commands (hurt/attack; slot <c>kAiBlackboardIntSprite2DCombatCommand</c>). <c>SpriteAnimatorComponent::IsCurrentClipFinished</c> for non-loop overlays. Platformer demo: J/K + locomotion via speed. |
| **P1.4** | **Hitboxes / attack arcs** | Implemented | `PhysicsQueries2D.hpp`: **`QueryOverlapCircleDynamics2D`** (P0.4-style dynamic broad-phase + circle narrow), **`QueryOverlapArcStatics2D`** / **`QueryOverlapArcDynamics2D`** (disk ∩ symmetric sector; direction normalized internally), convenience **`QueryOverlapArcWorldStatics2D`** / **`QueryOverlapArcWorldDynamics2D`**. `PhysicsQueryHitDynamic2D` for dynamic hits. Platformer demo: gems use **layer 1** static trigger circles + **layer 2** weapon query; **J** attack clip active frames run an arc overlap vs statics (P0.3) and collect gems; proximity collect unchanged. |

## P2 — Full vertical slice

| Priority | Feature | Status | Notes |
|----------|---------|--------|--------|
| **P2.1** | Inventory / equipment | Missing | UI + save + item defs (often game-specific). |
| **P2.2** | Dialogue / quests | Missing | Data-driven quests + UI; optional scripting hooks. |
| **P2.3** | Save/load game state | Partial | Asset paths and textures cached; no unified save blob API for ECS state. |
| **P2.4** | Minimap / fog | Missing | Render targets / secondary camera or UI overlay. |

## Design principles (SOLID + ECS)

- **Single responsibility**: Physics filtering lives in collision helpers + collider data; simulation stays in `PhysicsWorld2D`.
- **Open/closed**: New gameplay systems consume masks without editing the solver.
- **Interface segregation**: Colliders expose only category/mask accessors used by physics and queries.
- **Dependency inversion**: Higher-level combat/trigger systems depend on abstract overlap events, not raw collision internals.

Next incremental steps: **P1.2** gameplay vitals (`Health` / `Damage`), richer combat reactions on hit.
