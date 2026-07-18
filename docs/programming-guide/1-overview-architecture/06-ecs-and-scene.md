---
title: ECS and Scene
order: 6
---

# ECS and Scene

## Class Design: `GameObject`

`GameObject` is Spark's **entity**. It does not use inheritance for gameplay — behavior is composed from components.

| Field / API | Purpose |
|-------------|---------|
| `GetId()` | Stable `uint64_t` identifier |
| `GetName()` | `Utf8String` debug label |
| `GetParent()` / `SetParent()` | Scene hierarchy |
| `GetWorldMatrix()` | Composed TRS from root |
| `AddComponent<T>()` | Type-safe component factory |
| `GetComponent<T>()` | Lookup by `T::TypeKind` |
| `EmitSignal()` | Sibling messaging on same entity |

```cpp
template<typename T, typename... Args>
T* AddComponent(Args&&... args) {
    static_assert(std::is_base_of_v<GameComponent, T>);
    auto ptr = MakeUnique<T>(Forward<Args>(args)...);
    T* raw = ptr.Get();
    raw->InternalSetOwner(this);
    components.PushBack(UniquePtr<GameComponent>(ptr.Release()));
    raw->OnAttach(*this);
    return raw;
}
```

## Class Design: `GameComponent`

```cpp
class GameComponent {
public:
    virtual ComponentKind Kind() const noexcept = 0;
    virtual void OnAttach(GameObject& owner);
    virtual void OnDetach(GameObject& owner);
    virtual void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context);
    virtual void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload);
    virtual int UpdatePriority() const noexcept { return 0; }
};
```

Built-in priorities (`ComponentUpdatePriority`):

| Priority | Value | Examples |
|----------|-------|----------|
| `Billboard` | 50 | Face camera before gameplay |
| `AnimationDriver` | 100 | `Character3DAnimFsm`, `Sprite2DCharacterAnimFsm` |
| `AnimatorPlayback` | 200 | `Animator`, `SpriteAnimator` |
| — | 210 | `AnimationEventReceiver` |
| — | 250 | `AttachmentSocket` |
| — | 295 | `SpringArm3D` |
| — | 300 | `Camera2DRig`, `CameraFollow3D` |

Full per-component tables: [Game Component Reference](07-game-component-reference.html).

## Class Design: `GameWorld`

`GameWorld` owns all `GameObject` instances and **asset caches**:

```cpp
GameObject* CreateGameObject();
void DestroyGameObject(GameObject* object);
bool SetParent(GameObject* child, GameObject* newParent);
void UpdateGameObjects(const FrameTiming& timing, IEngineContext& context);

// Asset cache
void RegisterMesh(const SharedPtr<Mesh>& mesh, const char* key);
SharedPtr<Mesh> FindMesh(const char* key) const;
GltfAsset LoadGltf(const char* path);
SkinnedGltfAsset LoadSkinnedGltf(const char* path);
void SetUiFont(const SharedPtr<Font>& font);
```

## Class Design: `Scene`

`Scene` is a **read-only query layer** over `GameWorld`:

```cpp
void ForEachDrawable(const DrawableFn& fn);
void ForEachSkinnedDrawable(const SkinnedFn& fn);
void ForEachSprite(const SpriteFn& fn);
void ForEachTilemap(const TilemapFn& fn);
void ForEachPointLight(const PointLightFn& fn);
void ForEachGuiCanvas(const GuiCanvasFn& fn);
void SetSpatialPartitionKind(ScenePartitionKind kind);  // UniformGrid or BVH
```

## Signals Example

```cpp
// TransformComponent emits SignalId::TransformChanged
void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override {
    if (id == SignalId::TransformChanged) {
        // Rebuild local bounds, etc.
    }
    if (id == SignalId::Physics2DTriggerOverlap) {
        GameObject* other = static_cast<GameObject*>(payload.ptr);
        (void)other;
    }
    if (id == SignalId::Physics3DTriggerEnter) {
        GameObject* other = static_cast<GameObject*>(payload.ptr);
        (void)other;
    }
    if (id == SignalId::AnimationEvent) {
        const char* eventName = static_cast<const char*>(payload.ptr);
        (void)eventName;
    }
    if (id == SignalId::DamageApplied) {
        const auto* dmg = static_cast<const DamageSignalPayload*>(payload.ptr);
        (void)dmg;
    }
}
```

See [Game Component Reference](07-game-component-reference.html) for every component and signal payload.

## Rendering Hand-off

```cpp
Spark::SceneRenderParams params{};
ProcessTimeOfDayDrivers(world, timing.deltaTimeSeconds);  // optional if not using FillStandardLitSceneFromWorld
Spark::FillStandardLitSceneFromWorld(world, context, viewProj, camPos,
    lightDir, lightColor, lightIntensity, ambient, enableParticles,
    camRight, camUp, sceneTime, params);  // applies TimeOfDayDriver + fog/post volumes internally
Spark::PaintGuiCanvases(world, params, fbW, fbH);
context.SetSceneRenderParams(params);
```

Part 1 complete → [Game Component Reference](07-game-component-reference.html) · **Part 2**: [Sprites](2d-graphics/01-sprites.html).
