# Meshes and Materials

## Class Design: `Mesh`

CPU vertex/index buffers. Loaders and primitives live on `Mesh` (`spark/scene/Mesh.hpp`):

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

For assets with multiple material slots per mesh:

```cpp
GltfAsset building = world.LoadGltf("assets/models/Building.glb");
auto* go = world.CreateGameObject();
go->AddComponent<TransformComponent>();
go->AddComponent<MeshComponent>(building.mesh, SceneMeshSlot::Custom, Vector3::One);
auto* multi = go->AddComponent<MultiMaterialComponent>();
multi->PopulateFromGltfAsset(building);
```

`ThreeDDemo` and `SkyDemo` use `MultiMaterialComponent` for glTF props with per-submesh textures.

## Spawn a Lit glTF Prop

```cpp
GltfAsset asset = world.LoadGltf("assets/models/Crate.glb");
auto* go = world.CreateGameObject();
go->AddComponent<TransformComponent>()->SetTranslation({0, 0, 0});
go->AddComponent<MeshComponent>(asset.mesh, SceneMeshSlot::Custom, Vector3::One);

auto* mat = go->AddComponent<MaterialComponent>(asset.baseColorTexture);
mat->SetRoughness(0.55F);
mat->SetMetallic(0.1F);
mat->SetShadingModel(SceneShadingModel::LitPbr);
```

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
