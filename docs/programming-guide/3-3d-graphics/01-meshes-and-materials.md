# Meshes and Materials

## Class Design: `Mesh`

CPU vertex/index buffers. Loaders and primitives live on `Mesh` (`spark/scene/mesh/Mesh.hpp`):

```cpp
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
};

static Mesh CreateUnitCube();
static Mesh CreateGroundPlane(float halfExtent);
static Mesh CreateSkyDome(float radius, int latSegs, int lonSegs);
static bool TryLoadFromObj(const char* path, Mesh& outMesh);
static bool TryLoadFromGltf(const char* path, Mesh& outMesh, SharedPtr<Texture2D>* outBaseColor = nullptr);
```

## Class Design: `MeshComponent`

```cpp
MeshComponent(SharedPtr<Mesh> mesh, SceneMeshSlot slot, Vector3 albedo);
void SetMesh(SharedPtr<Mesh> m);
void SetMeshSlot(SceneMeshSlot slot);
```

| `SceneMeshSlot` | Use |
|-----------------|-----|
| `UnitCube` | Built-in GPU cube |
| `GroundPlane` | Built-in plane |
| `Custom` | Upload `Mesh` vertices each frame |

## Class Design: `MaterialComponent`

```cpp
class MaterialComponent final : public GameComponent {
public:
    MaterialComponent(SharedPtr<Texture2D> baseColor, Vector3 inTint = Vector3::One);
    void SetMetallic(float m);
    void SetRoughness(float r);
    void SetEmissive(const Vector3& rgb, float intensity);
    void SetShadingModel(SceneShadingModel s);  // LitPbr, ToonCel, ...
    SharedPtr<Texture2D> GetNormalTexture() const noexcept;
    SharedPtr<Texture2D> GetMetallicRoughnessTexture() const noexcept;
};
```

`ApplyMaterialComponentToSceneDrawItem()` copies PBR fields into `SceneDrawItem`.

## Multi-Material glTF

For assets with multiple material slots per mesh, prefer `GltfAssetBinder` so slots bind to shared library keys (`path.glb#material/N`):

```cpp
auto* go = world.CreateGameObject();
go->AddComponent<TransformComponent>();
GltfAssetBinder::BindFromPath(*go, "assets/models/Building.glb");
```

`BindFromPath` probes the file, loads rigid or skinned content automatically, and attaches mesh + materials (+ skeleton for skinned bind-pose display). For explicit control, use `BindRigidMesh` / `BindSkinnedMesh` with a known asset type.

For inline-only setup (no library retain), `PopulateFromGltfAsset` still copies textures and factors directly into slot storage.

`ThreeDDemo` and `SkyDemo` use multi-material glTF props with library keys when loaded through `GltfAssetBinder`.

## Material library (`.sparkmat`)

Author reusable materials under `assets/materials/*.sparkmat`. Components reference them by key:

```cpp
material->SetMaterialAsset(world, "materials/hero.sparkmat");
multi->SetSlotMaterialAsset(world, 0, "materials/trim.sparkmat");
```

glTF imports also expose `models/Foo.glb#material/0` keys. `.sparkmat` files persist full PBR scalars, shading model, and toon fields (`sparkmat_v1` + slot v1 payload).

## Spawn a Lit glTF Prop

Prefer `GltfAssetBinder` for display-ready binding (mesh + material maps + library keys):

```cpp
GltfAssetBinder::BindFromPath(*go, "assets/models/DamagedHelmet.glb");
```

Or load and bind manually:

```cpp
GltfAsset asset = world.LoadGltf("assets/models/Crate.glb");
auto* go = world.CreateGameObject();
go->AddComponent<TransformComponent>()->SetTranslation({0, 0, 0});
GltfAssetBinder::BindRigidMesh(*go, asset, SceneMeshSlot::Custom, Vector3::One, "assets/models/Crate.glb");
if (MaterialComponent* mat = go->GetComponent<MaterialComponent>()) {
    ApplyGltfMaterialDesc(*mat, asset.material);
}
```

Imported meshes include tangent space for normal maps (`Mesh::RecomputeTangentSpace` on glTF load). See launcher **#21** / key **Q** (`GltfSamples3DDemo`) for a full PBR + HDR IBL reference scene.

## Procedural Cube (FPS Sample Style)

```cpp
auto unitCube = MakeShared<Mesh>(Mesh::CreateUnitCube());
world.RegisterMesh(unitCube, "fps/unit_cube");

auto* target = world.CreateGameObject();
target->AddComponent<TransformComponent>()->SetTranslation({5, 1, -3});
target->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::Custom, Vector3{0.9F, 0.3F, 0.2F});
target->AddComponent<MaterialComponent>(nullptr)->SetRoughness(0.35F);
```

Next: [Cameras in 3D](02-cameras-3d.md).
