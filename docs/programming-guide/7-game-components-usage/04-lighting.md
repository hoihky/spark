# Component Guide: Lighting

Lights are **ECS components** gathered in `FillStandardLitSceneFromWorld` into `SceneRenderParams::pointLights` / `spotLights`. The directional sun usually comes from submit arguments or `DirectionalLightComponent`.

2D-specific: `PointLight2DComponent` and `SpriteLighting2DComponent` (see [2D rendering guide](01-2d-rendering.md)).

| Component | Role |
|-----------|------|
| `DirectionalLightComponent` | Override sun |
| `PointLightComponent` | 3D punctual (+ optional shadow) |
| `PointLight2DComponent` | XY punctual, no shadow |
| `SpotLightComponent` | Cone light (+ optional shadow) |

---

## DirectionalLightComponent

**Header:** `spark/ecs/components/lighting/DirectionalLightComponent.hpp`

#### C++

```cpp
auto* sunGo = world.CreateGameObject();
auto* dl = sunGo->AddComponent<DirectionalLightComponent>();
dl->SetDirection(Vector3{0.25F, -1.0F, 0.15F}.Normalized());
dl->SetColor(Vector3{1.0F, 0.97F, 0.92F});
dl->SetIntensity(3.2F);
```

#### C\#

```csharp
var dl = sunGo.AddDirectionalLight(new Vector3(1f, 0.97f, 0.92f), intensity: 3.2f);
dl.SetDirection(sunDir);
```

#### Python

```python
sun_go.get_or_add_default_component(ComponentKind.DirectionalLight)
```

---

## PointLightComponent

#### C++

```cpp
lamp->AddComponent<TransformComponent>()->SetTranslation({3, 2, 1});
PointLightComponent* pl = lamp->AddComponent<PointLightComponent>();
pl->SetColor({1.0F, 0.85F, 0.6F});
pl->SetIntensity(4.0F);
pl->SetRange(12.0F);
pl->SetCastsShadow(true);
```

#### C\#

```csharp
var pl = lamp.AddPointLight(new Vector3(1f, 0.85f, 0.6f), 4f, 12f);
pl.SetCastsShadow(true);
```

#### Python

```python
lamp.get_or_add_default_component(ComponentKind.PointLight)
```

---

## PointLight2DComponent

**Header:** `spark/ecs/components/lighting/PointLight2DComponent.hpp`

#### C++

```cpp
lantern->AddComponent<PointLight2DComponent>(Vector3{1, 0.9F, 0.7F}, 2.2F, 8.0F)
    ->SetLocalOffset({0.0F, 0.4F});
```

#### C\#

```csharp
lantern.AddPointLight2D(new Vector3(1, 0.9f, 0.7f), 2.2f, 8f);
```

#### Python

```python
lantern.get_or_add_default_component(ComponentKind.PointLight2D)
```

---

## SpotLightComponent

Local **−Z** is beam axis — orient transform accordingly.

#### C++

```cpp
spotGo->AddComponent<TransformComponent>()->SetRotationEuler({DegreesToRadians(-45.0F), 0, 0});
SpotLightComponent* sl = spotGo->AddComponent<SpotLightComponent>();
sl->SetOuterConeDegrees(28.0F);
sl->SetCastsShadow(true);
```

#### C\#

```csharp
var sl = spotGo.AddSpotLight(color, intensity, range);
sl.SetOuterConeDegrees(28f);
```

#### Python

```python
spot_go.get_or_add_default_component(ComponentKind.SpotLight)
```

---

## Material emissive (non-component light)

Glow without a light entity:

#### C++

```cpp
mat->SetEmissive({0.2F, 0.9F, 1.0F}, 2.5F);
```

#### C\#

```csharp
go.AddMaterialEmissive(new Vector3(0.2f, 0.9f, 1f), 2.5f);
```

#### Python

```python
mat = go.add_material_lit("", Vector3(1, 1, 1))
```

Next: [Camera components](05-camera.md).
