# Assets and content pipeline

Games resolve **meshes, textures, sounds, and glTF** through `GameWorld` registration and async loading. Built assets land under the CMake `SPARK_BUILD_ASSETS_DIR` at build time; keys are logical strings (not always absolute paths).

See also: [Building and running](03-building-and-running.md) · [Blender glTF export](../../BLENDER_GLTF_ANIMATION_EXPORT.md) · external [Materials and lighting](../../MATERIALS_AND_LIGHTING.md).

## Path roots

| Mechanism | Use |
|-----------|-----|
| `SPARK_BUILD_ASSETS_DIR` | Runtime root for bundled `assets/` |
| Relative keys | `"sprites/foo.png"`, `"assets/models/Fox.glb"` in registrars |
| `Utf8String(SPARK_BUILD_ASSETS_DIR).AppendUtf8(...)` | C++ absolute fallback |

### C++

```cpp
#include "spark/config.hpp"
Utf8String path = Utf8String(SPARK_BUILD_ASSETS_DIR).AppendUtf8("/sprites/hero.png");
```

### C\#

```csharp
// SparkScriptHost resolves asset roots from native bootstrap; use keys under assets/.
world.RegisterTextureFromFile("game/hero", "assets/sprites/hero.png");
```

### Python

```python
# Keys like "sprites/foo.png" resolve relative to engine asset dir when registered.
world.register_checkerboard_texture("game/tiles", c0, c1)
```

## Concept: procedural fallbacks (demos)

When files are missing, demos often call `Texture2D::CreateCheckerboard`, `Mesh::CreateUnitCube`, or `ProceduralSoundPresets` — keep shipping paths and keys aligned before stripping procedural code.

## Concept: mesh registration

### C++

```cpp
SharedPtr<Mesh> cube = MakeShared<Mesh>(Mesh::CreateUnitCube());
world.RegisterMesh(cube, "fps/unit_cube");

go->AddComponent<MeshComponent>(cube, SceneMeshSlot::Custom, albedoTint);
```

### C\#

```csharp
world.RegisterMesh(Mesh.CreateUnitCube(), "fps/unit_cube");
go.AddMesh(SceneMeshSlot.Custom, albedo, "fps/unit_cube");
```

### Python

```python
world.register_unit_cube_mesh("fps/unit_cube")
```

## Concept: glTF async load

### C++

```cpp
world.RequestGltf("assets/models/Fox.glb");

void OnUpdate(const FrameTiming& timing, IEngineContext& ctx) override {
    world.PumpAssets();
    if (world.IsGltfReady("assets/models/Fox.glb")) {
        SkinnedGltfAsset fox = world.LoadSkinnedGltf("assets/models/Fox.glb");
        // attach SkinnedMeshComponent + AnimatorComponent once
    }
    Game::OnUpdate(timing, ctx);
}
```

Always **pump** on the main thread while loads are in flight (`ThreeDDemo`, `CharacterCameraDemo`, **#21**).

### C\#

```csharp
World.RequestGltf("assets/models/Fox.glb");
World.PumpAssets();
```

### Python

```python
# glTF pump/bindings partial — use C++ host or extend catalog when adding gameplay.
```

## Concept: textures and audio

| Asset | Typical API |
|-------|-------------|
| 2D texture | `world.RegisterTexture` / `RegisterTextureFromFile` |
| Sound | `SoundClip::TryLoadFromBundledAsset` — [Clips](../6-sound/02-clips.md) |
| `.sparkvfx` | `VfxPlayerComponent` — [VFX](../3-3d-graphics/08-vfx-and-sparkvfx.md) |

## Shipping checklist

- [ ] Copy or install `assets/` next to the executable (CMake install rules / `SPARK_BUILD_ASSETS_DIR`).  
- [ ] Register meshes/textures in `OnAttach` before entities reference keys.  
- [ ] Pump glTF until `IsGltfReady` before attaching skinned meshes.  
- [ ] Verify Release build paths (no debug-only relative `../` assumptions).  
- [ ] Regenerate C\# mirrors if you add catalog methods ([Scripting](13-scripting-and-bindings.md)).

Next: [Scripting and bindings](13-scripting-and-bindings.md).
