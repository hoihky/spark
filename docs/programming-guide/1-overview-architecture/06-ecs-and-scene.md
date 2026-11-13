# ECS and Scene

Spark ECS is **gameplay-oriented**: entities are `GameObject`, data and behavior are `GameComponent` derivatives looked up by `ComponentKind`. There is no archetype/query DSL — use `ForEachGameObjectWithComponents` and tags for iteration.

`Scene` is a **read/query façade** over the same `GameWorld` the game mutates; rendering uses `FillStandardLitSceneFromWorld(world, …)` rather than Scene iterators in most games.

## Entity: `GameObject`

| Field / API | Purpose |
|-------------|---------|
| `GetId()` | Stable `uint64_t` |
| `GetName()` | Debug `Utf8String` |
| `GetParent()` / `SetParent()` | Hierarchy |
| `GetWorldMatrix()` | Composed TRS from root |
| `AddComponent<T>()` | Factory + `OnAttach` |
| `GetComponent<T>()` | Lookup by `T::TypeKind` |
| `EmitSignal()` | Same-object component messaging |
| `SetTag` / queries | Lightweight grouping |

```cpp
template<typename T, typename... Args>
T* AddComponent(Args&&... args);
```

## Component base: `GameComponent`

```cpp
virtual void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context);
virtual void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload);
virtual int UpdatePriority() const noexcept { return 0; }
```

Each `GameObject` **stable-sorts** components by `UpdatePriority()` before `OnUpdate`.

| Priority | Examples |
|----------|----------|
| 50 | `PlayerInputComponent`, `BillboardComponent` |
| 100 | Character / sprite FSM drivers |
| 200 | `AnimatorComponent`, `SpriteAnimatorComponent` |
| 215 | Sprite animation events, `AnimationHitbox2D` |
| 290–300 | Parallax, screen shake, camera rigs |

Full table: [Game Component Reference](07-game-component-reference.md#update-priorities).

## `GameWorld` ownership

```cpp
GameObject* CreateGameObject();
void DestroyGameObject(GameObject* object);
bool SetParent(GameObject* child, GameObject* newParent);
void UpdateGameObjects(const FrameTiming& timing, IEngineContext& context);
```

**Assets:** `world.Assets()` (`WorldAssets`) — `RegisterMesh`, `RegisterTexture`, `LoadGltf`, `LoadSkinnedGltf`, async `RequestGltf` + `Pump`, lookups by key/path. **UI fonts** stay on `GameWorld`: `SetUiFont` / `SetUiBoldFont`.

## Concept: create a player entity

### C++

```cpp
GameObject* player = world.CreateGameObject();
player->SetTag("player");
TransformComponent* tr = player->AddComponent<TransformComponent>();
tr->SetTranslation({0.0F, 1.0F, 0.0F});
player->AddComponent<Rigidbody2DComponent>();
player->AddComponent<SpriteComponent>(tex, tint, uv, sortOrder);
```

### C\#

```csharp
var player = world.CreateGameObject("Player");
player.SetTag("player");
player.GetOrAddTransform().SetTranslation(new Vector3(0, 1, 0));
player.AddRigidbody2D();
player.AddSprite("game/hero", tint, uv, sortOrder: 100);
```

### Python

```python
player = world.create_game_object("Player")
player.set_tag("player")
player.get_or_add_transform().set_translation(Vector3(0, 1, 0))
player.add_rigidbody_2d()
player.add_sprite("game/hero", tint, uv, sort_order=100)
```

## Concept: hierarchy

Parent transforms compose automatically:

### C++

```cpp
GameObject* root = world.CreateGameObject();
GameObject* child = world.CreateGameObject();
world.SetParent(child, root);
root->AddComponent<TransformComponent>()->SetTranslation({10, 0, 0});
child->AddComponent<TransformComponent>()->SetTranslation({0, 2, 0});
// child world ≈ (10, 2, 0)
```

### C\#

```csharp
world.SetParent(child, root);
root.GetOrAddTransform().SetTranslation(new Vector3(10, 0, 0));
child.GetOrAddTransform().SetTranslation(new Vector3(0, 2, 0));
```

### Python

```python
world.set_parent(child, root)
root.get_or_add_transform().set_translation(Vector3(10, 0, 0))
child.get_or_add_transform().set_translation(Vector3(0, 2, 0))
```

## Concept: queries and tags

### C++

```cpp
GameObject* player = world.FindGameObjectWithTag("player");

world.ForEachGameObjectWithComponents<MeshComponent, MaterialComponent>(
    [](GameObject* go) { /* lit mesh */ });

GameObject* camGo = world.FindFirstGameObjectWithComponents<Camera2DComponent>();
```

### C\#

```csharp
var player = world.FindGameObjectWithTag("player");
world.ForEachWithComponents<MeshComponent, MaterialComponent>(go => { });
```

### Python

```python
player = world.find_game_object_with_tag("player")
```

## Concept: signals (sibling-only)

`EmitSignal` dispatches to **components on the same `GameObject`**, not children. Example: goal trigger receives `Physics2DTriggerEnter` on the trigger object — put `GameFlowTriggerComponent` there, not on the player.

### C++

```cpp
void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override {
    if (id == SignalId::Physics2DTriggerEnter) {
        GameObject* other = static_cast<GameObject*>(payload.ptr);
    }
    if (id == SignalId::InputActionTriggered) {
        const char* action = static_cast<const char*>(payload.ptr);
        const auto phase = static_cast<InputActionPhase>(payload.a);
    }
    if (id == SignalId::DamageApplied) {
        const auto* dmg = static_cast<const DamageSignalPayload*>(payload.ptr);
    }
}
```

### C\#

```csharp
public override void OnSignal(GameObject owner, SignalId id, in SignalPayload payload)
{
    if (id == SignalId.Physics2DTriggerEnter) { /* ... */ }
}
```

### Python

```python
def on_signal(self, owner, signal_id, payload):
    if signal_id == SignalId.PHYSICS_2D_TRIGGER_ENTER:
        other = payload.ptr
```

Payload tables: [Game Component Reference — Signals](07-game-component-reference.md#signals-sparkecssignalhpp).

## `Scene` query layer

```cpp
void ForEachDrawable(const DrawableFn& fn);
void ForEachSkinnedDrawable(const SkinnedFn& fn);
void ForEachSprite(const SpriteFn& fn);
void SetSpatialPartitionKind(ScenePartitionKind kind);
```

Use for editor tooling, culling experiments, or custom render paths — production games usually call `FillStandardLitSceneFromWorld` directly on `GameWorld`.

## Concept: full render hand-off

### C++

```cpp
SceneRenderParams params{};
FillStandardLitSceneFromWorld(world, context, viewProj, camPos,
    lightDir, lightColor, lightIntensity, ambient, enableParticles,
    camRight, camUp, sceneTime, params);
PaintUiCanvases(world, params, fbW, fbH);
context.SetSceneRenderParams(params);
```

### C\#

```csharp
var p = new SceneRenderParams();
FillStandardLitSceneFromWorld(World, ctx, vp, camPos, ... , p);
PaintUiCanvases(World, p, fbW, fbH);
ctx.SetSceneRenderParams(p);
```

### Python

```python
# Host assembles params after on_update; call paint helpers when bound.
```

Part 1 foundations → [Game Component Reference](07-game-component-reference.md) · **Part 2**: [Sprites](../2-2d-graphics/01-sprites.md).
