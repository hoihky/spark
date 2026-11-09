# Component Guide: Camera

Cameras supply **`viewProjection`** and **`cameraPositionWorld`** to scene submit. 2D and 3D rigs use different component sets but share the rule: highest `priority` wins for `TryResolveMainCamera`.

| Component | Space |
|-----------|-------|
| `Camera2DComponent` | Orthographic XY |
| `Camera2DRigComponent` | Follow / bounds |
| `CameraBounds2DComponent` | Clamp rig |
| `ScreenShakeComponent` | Offset shake |
| `CameraComponent` | Perspective / ortho 3D |
| `SpringArm3DComponent` | Orbit arm |
| `CameraFollow3DComponent` | Look-at smooth |

---

## Camera2DComponent

#### C++

```cpp
auto* cam = cameraGo->AddComponent<Camera2DComponent>();
cam->SetHalfExtentY(6.5F);
cam->SetPriority(10);
```

#### C\#

```csharp
cameraGo.AddCamera2D(6.5f, priority: 10);
```

#### Python

```python
camera_go.add_camera_2d(half_extent_y=6.5, priority=10)
```

---

## Camera2DRigComponent

**Priority 300** — run after parallax (290).

#### C++

```cpp
auto* rig = cameraGo->AddComponent<Camera2DRigComponent>();
rig->SetMode(Camera2DRigMode::FollowTarget);
rig->SetTarget(player);
rig->SetFollowSmoothRate(8.0F);
```

#### C\#

```csharp
var rig = cameraGo.AddCamera2DRig();
rig.SetTarget(player);
rig.SetFollowSmoothRate(8f);
```

#### Python

```python
rig = camera_go.get_or_add_default_component(ComponentKind.Camera2DRig)
```

---

## CameraBounds2DComponent

Clamp rig position to world AABB.

#### C++

```cpp
cameraGo->AddComponent<CameraBounds2DComponent>()->SetBoundsMinMax({-50, -10}, {50, 30});
```

#### C\#

```csharp
cameraGo.GetOrAddComponent<CameraBounds2DComponent>()?.SetBoundsMinMax(min, max);
```

#### Python

```python
camera_go.get_or_add_default_component(ComponentKind.CameraBounds2D)
```

---

## ScreenShakeComponent

**Priority 295**

#### C++

```cpp
cameraGo->AddComponent<ScreenShakeComponent>()->AddTrauma(0.4F);
```

#### C\#

```csharp
cameraGo.GetComponent<ScreenShakeComponent>()?.AddTrauma(0.4f);
```

#### Python

```python
camera_go.get_or_add_default_component(ComponentKind.ScreenShake)
```

---

## CameraComponent (3D)

#### C++

```cpp
auto* cam = camGo->AddComponent<CameraComponent>();
cam->SetProjectionMode(CameraProjectionMode::Perspective);
cam->SetFovYDegrees(72.0F);
cam->SetPriority(10);
```

#### C\#

```csharp
camGo.AddCamera().SetFovYDegrees(72f);
```

#### Python

```python
cam_go.get_or_add_default_component(ComponentKind.Camera)
```

---

## SpringArm3DComponent + CameraFollow3DComponent

#### C++

```cpp
auto* arm = camGo->AddComponent<SpringArm3DComponent>();
arm->SetPivotTarget(player);
arm->SetArmLength(4.5F);
camGo->AddComponent<CameraFollow3DComponent>()->SetTarget(player);
```

#### C\#

```csharp
camGo.AddSpringArm3D().SetPivotTarget(player);
camGo.AddCameraFollow3D().SetTarget(player);
```

#### Python

```python
cam_go.get_or_add_default_component(ComponentKind.SpringArm3D)
```

---

## Submit with main camera

#### C++

```cpp
SubmitStandardLitSceneFromWorldWithCamera(world, ctx, sunDir, sunColor, 1.0F, ambient, false, t);
```

#### C\#

```csharp
SceneSubmit.SubmitStandardLitSceneFromWorldWithCamera(World, ctx, ...);
```

#### Python

```python
# Native template submits after on_update when Camera2DComponent exists.
```

Next: [Physics 2D components](06-physics-2d.md).
