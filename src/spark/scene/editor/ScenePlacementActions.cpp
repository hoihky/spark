#include "spark/scene/editor/ScenePlacementActions.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/world/SpawnPointComponent.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"
#include "spark/scene/core/SceneManager.hpp"
#include "spark/scene/prefab/PrefabInstantiator.hpp"

#include <cmath>
#include <cstring>
#include <utility>

namespace Spark {

namespace {

void ReportStatus(ScenePlacementContext& ctx, const char* message) {
    if (ctx.setStatus != nullptr) {
        ctx.setStatus(message, ctx.statusUserData);
    }
}

[[nodiscard]] const char* MeshPresetRelPath(const int presetIndex) noexcept {
    static constexpr const char* kPaths[] = {
            "models/DamagedHelmet.glb",
            "models/SheenChair.glb",
            "builtin:unit_cube",
    };
    if (presetIndex < 0 || presetIndex >= 3) {
        return kPaths[0];
    }
    return kPaths[presetIndex];
}

void LightPresetParams(const int preset, Vector3& outColor, float& outIntensity, float& outRange) noexcept {
    switch (preset) {
    case 1:
        outColor = {0.88F, 0.94F, 1.0F};
        outIntensity = 3.2F;
        outRange = 18.0F;
        break;
    case 2:
        outColor = {0.95F, 0.55F, 0.92F};
        outIntensity = 2.8F;
        outRange = 14.0F;
        break;
    case 0:
    default:
        outColor = {1.0F, 0.88F, 0.72F};
        outIntensity = 3.5F;
        outRange = 14.0F;
        break;
    }
}

class MeshPresetPlacementAction final : public IScenePlacementAction {
public:
    explicit MeshPresetPlacementAction(int presetIndex, Utf8String label) : preset(presetIndex), menuLabel(MoveTemp(label)) {}

    [[nodiscard]] Utf8String GetMenuLabel() const override { return menuLabel; }

    bool Execute(ScenePlacementContext& ctx) override {
        const char* rel = MeshPresetRelPath(preset);
        GameObject* object = ctx.world.CreateGameObject();
        object->GetName() = Utf8String("SceneEditorPlaced");
        TransformComponent* transform = object->AddComponent<TransformComponent>();

        if (std::strcmp(rel, "builtin:unit_cube") == 0) {
            const float uniformScale = 0.85F;
            transform->SetTranslation({ctx.groundHit.x, uniformScale, ctx.groundHit.z});
            transform->SetUniformScale(uniformScale);
            if (ctx.unitCubeAsset) {
                object->AddComponent<MeshComponent>(
                        ctx.unitCubeAsset, SceneMeshSlot::UnitCube, Vector3{0.72F, 0.58F, 0.42F});
            }
            if (MaterialComponent* material = object->AddComponent<MaterialComponent>()) {
                material->SetMetallic(0.04F);
                material->SetRoughness(0.55F);
            }
            ctx.content.TrackRoot(object);
            ctx.content.TrackPlaced(object, Utf8String("builtin:unit_cube"));
            ReportStatus(ctx, "Placed unit cube.");
            return true;
        }

        Utf8String full = ScenePathResolver::SourceAssetPath(nullptr, rel);
        GltfAsset gltf{};
        if (!ctx.world.AwaitGltf(full.CStr(), gltf) || !gltf.mesh) {
            ctx.world.DestroyGameObject(object);
            ReportStatus(ctx, "Could not load mesh preset.");
            return false;
        }
        Vector3 boundsMin{};
        Vector3 boundsMax{};
        float uniformScale = 1.8F;
        if (gltf.mesh->TryComputeAxisAlignedBounds(boundsMin, boundsMax)) {
            const float dx = boundsMax.x - boundsMin.x;
            const float dy = boundsMax.y - boundsMin.y;
            const float dz = boundsMax.z - boundsMin.z;
            const float maxExtent = std::max({dx, dy, dz});
            if (maxExtent > 1.0e-4F) {
                uniformScale = 2.2F / maxExtent;
            }
        }
        float faceCameraYaw = Spark::Pi;
        if (std::strstr(rel, "DamagedHelmet") != nullptr) {
            faceCameraYaw = 0.0F;
        }
        const Quaternion rotation = Quaternion::FromAxisAngle(Vector3::UnitY, faceCameraYaw);
        constexpr float kGroundClearance = 0.08F;
        const float yOnGround = -boundsMin.y * uniformScale + kGroundClearance;
        transform->SetUniformScale(uniformScale);
        transform->SetTranslation({ctx.groundHit.x, yOnGround, ctx.groundHit.z});
        transform->SetRotation(rotation);
        GltfAssetBinder::BindRigidMesh(
                *object, gltf, SceneMeshSlot::Custom, Vector3{1.0F, 1.0F, 1.0F}, full.CStr());
        ctx.content.TrackRoot(object);
        ctx.content.TrackPlaced(object, Utf8String(rel));
        ReportStatus(ctx, "Placed mesh preset.");
        return true;
    }

private:
    int preset = 0;
    Utf8String menuLabel{};
};

class PrefabPlacementAction final : public IScenePlacementAction {
public:
    explicit PrefabPlacementAction(const PrefabCatalogEntry& entry) : catalogEntry(entry) {}

    [[nodiscard]] Utf8String GetMenuLabel() const override { return catalogEntry.menuLabel; }

    bool Execute(ScenePlacementContext& ctx) override {
        const Utf8String path = ScenePathResolver::BuildRuntimePath("prefabs", catalogEntry.fileName.CStr());
        PrefabInstantiateOptions options{};
        options.assetsRoot = ScenePathResolver::AssetsRoot();
        options.position = {ctx.groundHit.x, 0.0F, ctx.groundHit.z};
        options.additive = true;
        options.pumpUntilReady = true;

        const PrefabInstantiateResult result = ctx.sceneManager.InstantiatePrefab(path.CStr(), options);
        if (!result.ready || result.rootObjects.IsEmpty()) {
            ReportStatus(ctx, "Could not instantiate prefab.");
            return false;
        }

        ctx.instances.Register(result.instanceId);
        for (std::size_t i = 0; i < result.rootObjects.GetSize(); ++i) {
            GameObject* root = result.rootObjects[i];
            if (root == nullptr) {
                continue;
            }
            ctx.content.TrackRoot(root);
            ctx.content.TrackPlaced(root, catalogEntry.captureMeshHint);
        }
        ReportStatus(ctx, "Placed prefab.");
        return true;
    }

private:
    PrefabCatalogEntry catalogEntry{};
};

class SpawnPointPlacementAction final : public IScenePlacementAction {
public:
    [[nodiscard]] Utf8String GetMenuLabel() const override { return Utf8String("Spawn point — Player"); }

    bool Execute(ScenePlacementContext& ctx) override {
        GameObject* object = ctx.world.CreateGameObject();
        object->GetName() = Utf8String("PlayerSpawn");
        TransformComponent* transform = object->AddComponent<TransformComponent>();
        transform->SetTranslation({ctx.groundHit.x, 1.0F, ctx.groundHit.z});
        SpawnPointComponent* spawnPoint = object->AddComponent<SpawnPointComponent>();
        spawnPoint->SetSpawnName("Player");
        ctx.content.TrackRoot(object);
        ctx.content.TrackPlaced(object, Utf8String{});
        ReportStatus(ctx, "Placed Player spawn point.");
        return true;
    }
};

class LightPresetPlacementAction final : public IScenePlacementAction {
public:
    explicit LightPresetPlacementAction(int presetIndex, Utf8String label) : preset(presetIndex), menuLabel(MoveTemp(label)) {}

    [[nodiscard]] Utf8String GetMenuLabel() const override { return menuLabel; }

    bool Execute(ScenePlacementContext& ctx) override {
        if (ctx.content.GetUserLights().GetSize() >= 7) {
            ReportStatus(ctx, "Too many lights (max 7 user lights).");
            return false;
        }
        Vector3 color{};
        float intensity = 3.5F;
        float range = 14.0F;
        LightPresetParams(preset, color, intensity, range);

        GameObject* object = ctx.world.CreateGameObject();
        object->GetName() = Utf8String("SceneEditorLight");
        TransformComponent* transform = object->AddComponent<TransformComponent>();
        transform->SetTranslation({ctx.groundHit.x, 3.5F, ctx.groundHit.z});
        object->AddComponent<PointLightComponent>(color, intensity, range);
        if (ctx.unitCubeAsset) {
            object->AddComponent<MeshComponent>(
                    ctx.unitCubeAsset, SceneMeshSlot::UnitCube, Vector3{1.0F, 1.0F, 1.0F});
            if (MaterialComponent* material = object->AddComponent<MaterialComponent>()) {
                material->SetMetallic(0.12F);
                material->SetRoughness(0.35F);
                material->SetEmissive(color, 7.5F);
            }
            if (ctx.syncLightGizmoEmissive != nullptr) {
                ctx.syncLightGizmoEmissive(object);
            }
        }
        ctx.content.TrackRoot(object);
        ctx.content.TrackUserLight(object);
        ReportStatus(ctx, "Placed light.");
        return true;
    }

private:
    int preset = 0;
    Utf8String menuLabel{};
};

class LambdaScenePlacementAction final : public IScenePlacementAction {
public:
    using ExecuteFn = bool (*)(ScenePlacementContext&);
    using AvailableFn = bool (*)(const ScenePlacementContext&);

    LambdaScenePlacementAction(Utf8String label, ExecuteFn executeFn, AvailableFn availableFn = nullptr)
            : menuLabel(MoveTemp(label)), execute(executeFn), available(availableFn) {}

    [[nodiscard]] Utf8String GetMenuLabel() const override { return menuLabel; }

    [[nodiscard]] bool IsAvailable(const ScenePlacementContext& ctx) const override {
        return available == nullptr || available(ctx);
    }

    bool Execute(ScenePlacementContext& ctx) override { return execute != nullptr && execute(ctx); }

private:
    Utf8String menuLabel{};
    ExecuteFn execute = nullptr;
    AvailableFn available = nullptr;
};

}  // namespace

namespace {

template<typename ActionT, typename... Args>
void RegisterConcreteAction(ScenePlacementActionRegistry& registry, Args&&... args) {
    UniquePtr<ActionT> concrete = MakeUnique<ActionT>(std::forward<Args>(args)...);
    registry.Register(UniquePtr<IScenePlacementAction>(static_cast<IScenePlacementAction*>(concrete.Release())));
}

}  // namespace

void ScenePlacementActionRegistry::Register(UniquePtr<IScenePlacementAction> action) {
    if (action) {
        actions.PushBack(MoveTemp(action));
    }
}

void ScenePlacementActionRegistry::Clear() noexcept {
    actions.Clear();
    visibleActionIndices.Clear();
}

void ScenePlacementActionRegistry::BuildMenu(Array<Utf8String>& outLabels, const ScenePlacementContext& ctx) {
    outLabels.Clear();
    visibleActionIndices.Clear();
    for (std::size_t i = 0; i < actions.GetSize(); ++i) {
        if (actions[i] != nullptr && actions[i]->IsAvailable(ctx)) {
            outLabels.PushBack(actions[i]->GetMenuLabel());
            visibleActionIndices.PushBack(i);
        }
    }
}

bool ScenePlacementActionRegistry::Execute(const std::size_t menuIndex, ScenePlacementContext& ctx) {
    if (menuIndex >= visibleActionIndices.GetSize()) {
        return false;
    }
    const std::size_t actionIndex = visibleActionIndices[menuIndex];
    if (actionIndex >= actions.GetSize() || actions[actionIndex] == nullptr) {
        return false;
    }
    return actions[actionIndex]->Execute(ctx);
}

void ScenePlacementActionRegistry::RegisterDemoDefaults(ScenePlacementActionRegistry& registry) {
    RegisterConcreteAction<MeshPresetPlacementAction>(registry, 0, Utf8String("Mesh — DamagedHelmet.glb"));
    RegisterConcreteAction<MeshPresetPlacementAction>(registry, 1, Utf8String("Mesh — SheenChair.glb"));
    RegisterConcreteAction<MeshPresetPlacementAction>(registry, 2, Utf8String("Mesh — Unit cube (builtin)"));
    RegisterConcreteAction<SpawnPointPlacementAction>(registry);
    RegisterConcreteAction<LightPresetPlacementAction>(registry, 0, Utf8String("Light — Warm tungsten"));
    RegisterConcreteAction<LightPresetPlacementAction>(registry, 1, Utf8String("Light — Cool daylight"));
    RegisterConcreteAction<LightPresetPlacementAction>(registry, 2, Utf8String("Light — Soft magenta accent"));
}

void ScenePlacementActionRegistry::RegisterPrefabActions(
        ScenePlacementActionRegistry& registry,
        const PrefabCatalog& catalog) {
    const Array<PrefabCatalogEntry>& entries = catalog.GetEntries();
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        RegisterPrefabAction(registry, entries[i]);
    }
}

void ScenePlacementActionRegistry::RegisterPrefabAction(
        ScenePlacementActionRegistry& registry,
        const PrefabCatalogEntry& entry) {
    RegisterConcreteAction<PrefabPlacementAction>(registry, entry);
}

UniquePtr<IScenePlacementAction> MakeLambdaPlacementAction(
        Utf8String label,
        bool (*executeFn)(ScenePlacementContext&),
        bool (*availableFn)(const ScenePlacementContext&)) {
    UniquePtr<LambdaScenePlacementAction> concrete =
            MakeUnique<LambdaScenePlacementAction>(MoveTemp(label), executeFn, availableFn);
    return UniquePtr<IScenePlacementAction>(static_cast<IScenePlacementAction*>(concrete.Release()));
}

}  // namespace Spark
