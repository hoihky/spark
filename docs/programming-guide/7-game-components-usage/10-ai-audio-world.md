# Component Guide: AI, Audio, and World

Navigation, perception, sound spatialization, scene policy, and content streaming.

**Related:** [AI overview](../4-ai/01-ai-overview.md), [Sound engine](../6-sound/01-sound-engine.md).

## AI and navigation

| Component | Role |
|-----------|------|
| `AiAgentComponent` | FSM / GOAP / steering host |
| `GridNavAgent2DComponent` | Tile grid requests |
| `GridPathFollower2DComponent` | Follow grid path |
| `GridNavTarget2DComponent` | Click-to-move target |
| `NavMeshAgentComponent` | 3D navmesh |
| `PatrolPathComponent` | Waypoint loop |
| `PerceptionSensorComponent` | Sight / hearing |

### AiAgentComponent

#### C++

```cpp
auto* ai = enemy->AddComponent<AiAgentComponent>();
ai->SetMaxSpeed(4.5F);
ai->SetFsmEnabled(true);
ai->SetGoapEnabled(true);
```

#### C\#

```csharp
enemy.GetOrAddComponent<AiAgentComponent>()?.SetMaxSpeed(4.5f);
```

#### Python

```python
enemy.get_or_add_default_component(ComponentKind.AiAgent)
```

### GridNavAgent2DComponent + GridPathFollower2DComponent

#### C++

```cpp
enemy->AddComponent<GridNavAgent2DComponent>();
enemy->AddComponent<GridPathFollower2DComponent>()->SetDestinationCell({12, 8});
```

#### C\#

```csharp
enemy.AddGridNavAgent2D();
enemy.GetOrAddComponent<GridPathFollower2DComponent>();
```

#### Python

```python
enemy.get_or_add_default_component(ComponentKind.GridNavAgent2D)
```

### NavMeshAgentComponent (3D)

#### C++

```cpp
guard->AddComponent<NavMeshAgentComponent>()->SetDestination({10.0F, 0.0F, 5.0F});
```

#### C\#

```csharp
guard.GetOrAddComponent<NavMeshAgentComponent>();
```

#### Python

```python
guard.get_or_add_default_component(ComponentKind.NavMeshAgent)
```

---

## Audio

| Component | Role |
|-----------|------|
| `SoundCueComponent` | Play clips / loops |
| `AudioListenerComponent` | 3D ear |
| `AmbientZoneComponent` | Reverb / bed |

### SoundCueComponent

#### C++

```cpp
#include "spark/ecs/components/audio/SoundCueComponent.hpp"

player->AddComponent<SoundCueComponent>()->PlayOneShot("sfx/jump");
```

#### C\#

```csharp
player.AddSoundCue().PlayOneShot("sfx/jump");
```

#### Python

```python
player.get_or_add_default_component(ComponentKind.SoundCue)
```

### AudioListenerComponent

Attach to camera object for 3D panning.

#### C++

```cpp
cameraGo->AddComponent<AudioListenerComponent>();
```

#### C\#

```csharp
cameraGo.GetOrAddComponent<AudioListenerComponent>();
```

#### Python

```python
camera_go.get_or_add_default_component(ComponentKind.AudioListener)
```

### AmbientZoneComponent

#### C++

```cpp
auto* zone = cave->AddComponent<AmbientZoneComponent>();
zone->SetHalfExtents({6.0F, 4.0F, 6.0F});
zone->SetVolumeScale(0.85F);
zone->SetLowPassAmount(0.35F);
```

#### C\#

```csharp
cave.GetOrAddComponent<AmbientZoneComponent>();
```

#### Python

```python
cave.get_or_add_default_component(ComponentKind.AmbientZone)
```

---

## World and streaming

| Component | Role |
|-----------|------|
| `SceneSpatialPolicyComponent` | BVH / quadtree culling |
| `TimeOfDayDriverComponent` | Sun animation |
| `SpawnPointComponent` / `SpawnPoint2DComponent` | Player spawn |
| `GltfSceneSourceComponent` | Scene glTF root |
| `GltfInstanceNodeComponent` | Prefab instance node |
| `UiCanvasComponent` | Retained UI (see [UI guide](../1-overview-architecture/08-ui-and-toolkits.md)) |

### SceneSpatialPolicyComponent

#### C++

```cpp
worldRoot->AddComponent<SceneSpatialPolicyComponent>()
    ->SetPartitionKind(ScenePartitionKind::QuadTree);
```

#### C\#

```csharp
worldRoot.GetOrAddComponent<SceneSpatialPolicyComponent>();
```

#### Python

```python
world_root.get_or_add_default_component(ComponentKind.SceneSpatialPolicy)
```

### TimeOfDayDriverComponent

#### C++

```cpp
sun->AddComponent<TimeOfDayDriverComponent>()->SetDayLengthSeconds(600.0F);
```

#### C\#

```csharp
sun.GetOrAddComponent<TimeOfDayDriverComponent>();
```

#### Python

```python
sun.get_or_add_default_component(ComponentKind.TimeOfDayDriver)
```

### SpawnPoint2DComponent

#### C++

```cpp
spawn->AddComponent<SpawnPoint2DComponent>()->SetFacingRadians(0.0F);
```

#### C\#

```csharp
spawn.GetOrAddComponent<SpawnPoint2DComponent>();
```

#### Python

```python
spawn.get_or_add_default_component(ComponentKind.SpawnPoint2D)
```

### GltfSceneSourceComponent

#### C++

```cpp
root->AddComponent<GltfSceneSourceComponent>()->SetGltfPath("assets/scenes/level.glb");
```

#### C\#

```csharp
root.GetOrAddComponent<GltfSceneSourceComponent>();
```

#### Python

```python
root.get_or_add_default_component(ComponentKind.GltfSceneSource)
```

---

## Python pattern note

When `GameObject.add_*` shorthands are missing, use:

```python
from spark._generated.component_kind import ComponentKind

comp = go.get_or_add_default_component(ComponentKind.SoundCue)
# Instance methods: see spark/components/*.py after generate_python_bindings.py
```

Regenerate Python/C\# mirrors when the catalog changes ([Building and running](../1-overview-architecture/03-building-and-running.md)).

---

**Part 7 complete** → [Part 8: 2D game systems map](../8-2d-game/00-2d-game-systems-map.md) · Full API tables: [Game component reference](../1-overview-architecture/07-game-component-reference.md).
