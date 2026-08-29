#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/world/SpawnPointComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/core/SceneInstanceTracker.hpp"
#include "spark/scene/core/SceneLoadSession.hpp"
#include "spark/scene/core/SceneManager.hpp"
#include "spark/scene/core/SpawnPointService.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/prefab/PrefabCatalog.hpp"
#include "spark/scene/prefab/PrefabInstantiator.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace {

constexpr const char* kSmokeSceneFile = "editor_smoke.sparkscene";

Spark::Utf8String SmokeScenePath() {
    return Spark::ScenePathResolver::BuildRuntimePath("scenes", kSmokeSceneFile);
}

Spark::GameObject* FindByName(Spark::GameWorld& world, const char* name) {
    Spark::GameObject* found = nullptr;
    world.ForEachGameObject([&](Spark::GameObject* object) {
        if (object != nullptr && object->GetName() == Spark::Utf8String(name)) {
            found = object;
        }
    });
    return found;
}

std::size_t CountRenderableComponentsInSubtree(const Spark::GameObject& root) {
    std::size_t count = 0U;
    if (root.GetComponent<Spark::MeshComponent>() != nullptr || root.GetComponent<Spark::SkinnedMeshComponent>() != nullptr) {
        ++count;
    }
    const Spark::Array<Spark::GameObject*>& children = root.GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            count += CountRenderableComponentsInSubtree(*children[i]);
        }
    }
    return count;
}

std::size_t CountSubtreeObjects(const Spark::GameObject& root) {
    std::size_t count = 1U;
    const Spark::Array<Spark::GameObject*>& children = root.GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] != nullptr) {
            count += CountSubtreeObjects(*children[i]);
        }
    }
    return count;
}

Spark::MaterialComponent* FindFirstMaterialDescendant(Spark::GameObject& root) {
    Spark::MaterialComponent* found = root.GetComponent<Spark::MaterialComponent>();
    if (found != nullptr) {
        return found;
    }
    const Spark::Array<Spark::GameObject*>& children = root.GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        if (children[i] == nullptr) {
            continue;
        }
        if (Spark::MaterialComponent* material = FindFirstMaterialDescendant(*children[i])) {
            return material;
        }
    }
    return nullptr;
}

/** Mirrors the editor's save/load/play scene pipeline without GLFW UI. */
class EditorSmokeHarness {
public:
    Spark::GameWorld world{};
    Spark::SceneManager sceneManager{world};
    Spark::SceneLoadSession loadSession{sceneManager};
    Spark::SceneEditorContentModel contentModel{};
    Spark::SceneInstanceTracker instanceTracker{};
    Spark::SharedPtr<Spark::Mesh> unitCubeAsset{};
    Spark::SceneInstanceId loadedSceneId = Spark::kInvalidSceneInstanceId;
    Spark::SceneDocument pendingLoadDocument{};
    bool sceneLoadInProgress = false;

    void SetupUnitCube() {
        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SmokeUnitCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        world.RegisterMesh(unitCubeAsset, "spark/scene_editor/unit_cube");
    }

    void UnloadEditorSceneContent() {
        instanceTracker.UnloadAll(sceneManager);
        instanceTracker.Clear();
        loadedSceneId = Spark::kInvalidSceneInstanceId;
        sceneLoadInProgress = false;
        pendingLoadDocument = Spark::SceneDocument{};
        contentModel.ClearManualObjects(world);
        contentModel.ClearLists();
    }

    bool LoadSceneFile(const char* fileName) {
        const Spark::Utf8String path = Spark::ScenePathResolver::BuildRuntimePath("scenes", fileName);
        if (!Spark::ScenePathResolver::FileExists(path.CStr())) {
            return false;
        }
        Spark::SceneDeserializer deserializer{};
        Spark::SceneDocument document{};
        if (!deserializer.ReadFromFile(path.CStr(), document)) {
            return false;
        }
        UnloadEditorSceneContent();
        pendingLoadDocument = document;
        Spark::SceneLoadOptions options{};
        options.assetsRoot = Spark::ScenePathResolver::AssetsRoot();
        options.additive = true;
        loadedSceneId = sceneManager.BeginLoadSceneAsync(document, path.CStr(), options);
        if (loadedSceneId == Spark::kInvalidSceneInstanceId) {
            pendingLoadDocument = Spark::SceneDocument{};
            return false;
        }
        sceneLoadInProgress = true;
        return PumpUntilReady();
    }

    bool PumpUntilReady() {
        constexpr int kMaxIterations = 100000;
        for (int i = 0; i < kMaxIterations; ++i) {
            sceneManager.Pump();
            if (!sceneLoadInProgress || loadedSceneId == Spark::kInvalidSceneInstanceId) {
                return true;
            }
            if (sceneManager.IsSceneReady(loadedSceneId)) {
                FinalizeAsyncSceneLoad();
                sceneLoadInProgress = false;
                return true;
            }
            if (sceneManager.HasSceneFailed(loadedSceneId)) {
                sceneManager.UnloadScene(loadedSceneId);
                loadedSceneId = Spark::kInvalidSceneInstanceId;
                sceneLoadInProgress = false;
                pendingLoadDocument = Spark::SceneDocument{};
                return false;
            }
        }
        return false;
    }

    void FinalizeAsyncSceneLoad() {
        if (loadedSceneId == Spark::kInvalidSceneInstanceId) {
            return;
        }
        Spark::SceneEditorContentBindingHooks hooks{};
        hooks.unitCubeAsset = &unitCubeAsset;
        contentModel.IntegrateLoadedInstance(world, loadedSceneId, pendingLoadDocument, hooks);
        instanceTracker.Register(loadedSceneId);
        pendingLoadDocument = Spark::SceneDocument{};
    }

    bool PlaceUnitCube(const Spark::Vector3& position) {
        Spark::GameObject* object = world.CreateGameObject();
        object->GetName() = Spark::Utf8String("PlacedCube");
        Spark::TransformComponent* transform = object->AddComponent<Spark::TransformComponent>();
        transform->SetTranslation(position);
        object->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.55F, 0.35F, 0.22F});
        contentModel.TrackRoot(object);
        contentModel.TrackPlaced(object, Spark::Utf8String("builtin:unit_cube"));
        return true;
    }

    bool PlaceCratePrefab(const Spark::Vector3& position) {
        const Spark::Utf8String path = Spark::ScenePathResolver::BuildRuntimePath("prefabs", "crate.sparkscene");
        Spark::PrefabInstantiateOptions options{};
        options.assetsRoot = Spark::ScenePathResolver::AssetsRoot();
        options.position = position;
        options.additive = true;
        options.pumpUntilReady = true;
        const Spark::PrefabInstantiateResult result = sceneManager.InstantiatePrefab(path.CStr(), options);
        if (!result.ready || result.rootObjects.IsEmpty()) {
            return false;
        }
        instanceTracker.Register(result.instanceId);
        for (std::size_t i = 0; i < result.rootObjects.GetSize(); ++i) {
            Spark::GameObject* root = result.rootObjects[i];
            if (root == nullptr) {
                continue;
            }
            contentModel.TrackRoot(root);
            contentModel.TrackPlaced(root, Spark::Utf8String("prefabs/crate.sparkscene"));
        }
        return true;
    }

    bool PlaceGltfPrefab(const char* prefabFileName, const Spark::Vector3& position) {
        const Spark::Utf8String path = Spark::ScenePathResolver::BuildRuntimePath("prefabs", prefabFileName);
        Spark::PrefabInstantiateOptions options{};
        options.assetsRoot = Spark::ScenePathResolver::AssetsRoot();
        options.position = position;
        options.additive = true;
        options.pumpUntilReady = true;
        const Spark::PrefabInstantiateResult result = sceneManager.InstantiatePrefab(path.CStr(), options);
        if (!result.ready || result.rootObjects.IsEmpty()) {
            return false;
        }
        instanceTracker.Register(result.instanceId);
        Spark::Utf8String relativePath = Spark::Utf8String("prefabs/");
        relativePath.AppendUtf8(prefabFileName);
        for (std::size_t i = 0; i < result.rootObjects.GetSize(); ++i) {
            Spark::GameObject* root = result.rootObjects[i];
            if (root == nullptr) {
                continue;
            }
            contentModel.TrackRoot(root);
            contentModel.TrackPlaced(root, relativePath);
        }
        return true;
    }

    [[nodiscard]] std::size_t SaveTrackedScene(const char* fileName) const {
        Spark::SceneSerializer serializer{};
        Spark::SceneCaptureContext captureCtx = contentModel.BuildCaptureContext();
        const auto includeEntity = [this](const Spark::GameObject* object) -> bool {
            return contentModel.ShouldCapture(object);
        };
        Spark::SceneDocument document = serializer.Capture(world, captureCtx, includeEntity);
        document.header.name = Spark::Utf8String("EditorSmoke");
        document.header.assetsRoot = Spark::Utf8String(Spark::ScenePathResolver::AssetsRoot());
        const Spark::Utf8String path = Spark::ScenePathResolver::BuildRuntimePath("scenes", fileName);
        EXPECT_TRUE(serializer.WriteToFile(document, path.CStr()));
        return document.entities.GetSize();
    }

    void ReloadLikePlayMode(const char* fileName) {
        UnloadEditorSceneContent();
        ASSERT_TRUE(LoadSceneFile(fileName));
    }
};

bool FileContains(const char* path, const char* needle) {
    if (path == nullptr || needle == nullptr) {
        return false;
    }
    std::FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    Spark::Array<char> buffer{};
    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    if (size <= 0) {
        std::fclose(file);
        return false;
    }
    buffer.Resize(static_cast<std::size_t>(size) + 1U);
    const std::size_t readBytes = std::fread(buffer.GetData(), 1, static_cast<std::size_t>(size), file);
    std::fclose(file);
    buffer[readBytes] = '\0';
    return std::strstr(buffer.GetData(), needle) != nullptr;
}

}  // namespace

TEST(SceneEditorSmokeTest, ArenaLoadPlacePrefabSaveReloadRoundTrip) {
    const Spark::Utf8String arenaPath = Spark::ScenePathResolver::BuildRuntimePath("scenes", "arena.sparkscene");
    struct stat arenaStat {};
    if (stat(arenaPath.CStr(), &arenaStat) != 0 || !S_ISREG(arenaStat.st_mode)) {
        GTEST_SKIP() << "arena.sparkscene not available in build assets";
    }

    std::remove(SmokeScenePath().CStr());

    EditorSmokeHarness harness{};
    harness.SetupUnitCube();
    ASSERT_TRUE(harness.LoadSceneFile("arena.sparkscene"));

    EXPECT_NE(FindByName(harness.world, "PlayerSpawn"), nullptr);
    EXPECT_NE(FindByName(harness.world, "CrateA"), nullptr);
    EXPECT_EQ(harness.contentModel.GetPlacedObjects().GetSize(), 4U);
    EXPECT_EQ(harness.contentModel.GetUserLights().GetSize(), 1U);

    ASSERT_TRUE(harness.PlaceUnitCube({3.0F, 0.425F, 2.0F}));
    EXPECT_EQ(harness.contentModel.GetPlacedObjects().GetSize(), 5U);
    EXPECT_NE(FindByName(harness.world, "PlacedCube"), nullptr);

    const std::size_t savedEntities = harness.SaveTrackedScene(kSmokeSceneFile);
    EXPECT_EQ(savedEntities, 6U);
    EXPECT_TRUE(FileContains(SmokeScenePath().CStr(), "spawn_point"));
    EXPECT_TRUE(FileContains(SmokeScenePath().CStr(), "point_light"));
    EXPECT_TRUE(FileContains(SmokeScenePath().CStr(), "mesh unit_cube"));

    harness.ReloadLikePlayMode(kSmokeSceneFile);
    EXPECT_EQ(harness.contentModel.GetPlacedObjects().GetSize(), 5U);
    EXPECT_EQ(harness.contentModel.GetUserLights().GetSize(), 1U);

    Spark::SpawnPointService spawnPoints{};
    const Spark::SceneSpawnPose spawn = spawnPoints.Resolve(harness.world, "Player");
    EXPECT_TRUE(spawn.found);
    EXPECT_NE(spawn.object, nullptr);
    EXPECT_STREQ(spawn.component->GetSpawnName().CStr(), "Player");

    const Spark::GameObject* placedCube = FindByName(harness.world, "PlacedCube");
    ASSERT_NE(placedCube, nullptr);
    const Spark::MeshComponent* mesh = placedCube->GetComponent<Spark::MeshComponent>();
    ASSERT_NE(mesh, nullptr);
    EXPECT_FLOAT_EQ(mesh->GetAlbedo().x, 0.55F);

    std::remove(SmokeScenePath().CStr());
}

TEST(SceneEditorSmokeTest, PlayModeReloadPreservesEditorScene) {
    const Spark::Utf8String arenaPath = Spark::ScenePathResolver::BuildRuntimePath("scenes", "arena.sparkscene");
    struct stat arenaStat {};
    if (stat(arenaPath.CStr(), &arenaStat) != 0 || !S_ISREG(arenaStat.st_mode)) {
        GTEST_SKIP() << "arena.sparkscene not available in build assets";
    }

    std::remove(SmokeScenePath().CStr());

    EditorSmokeHarness harness{};
    harness.SetupUnitCube();
    ASSERT_TRUE(harness.LoadSceneFile("arena.sparkscene"));
    ASSERT_TRUE(harness.PlaceUnitCube({1.5F, 0.425F, -1.0F}));
    const std::size_t savedEntities = harness.SaveTrackedScene(kSmokeSceneFile);
    ASSERT_EQ(savedEntities, 6U);

    const std::size_t placedBeforePlay = harness.contentModel.GetPlacedObjects().GetSize();
    const std::size_t lightsBeforePlay = harness.contentModel.GetUserLights().GetSize();

    (void)harness.SaveTrackedScene(kSmokeSceneFile);
    harness.ReloadLikePlayMode(kSmokeSceneFile);

    Spark::SpawnPointService spawnPoints{};
    EXPECT_TRUE(spawnPoints.Resolve(harness.world, "Player").found);
    EXPECT_EQ(harness.contentModel.GetPlacedObjects().GetSize(), placedBeforePlay);
    EXPECT_EQ(harness.contentModel.GetUserLights().GetSize(), lightsBeforePlay);

    harness.ReloadLikePlayMode(kSmokeSceneFile);
    EXPECT_EQ(harness.contentModel.GetPlacedObjects().GetSize(), placedBeforePlay);
    EXPECT_NE(FindByName(harness.world, "CrateA"), nullptr);
    EXPECT_NE(FindByName(harness.world, "PlacedCube"), nullptr);

    std::remove(SmokeScenePath().CStr());
}

TEST(SceneEditorSmokeTest, PrefabSaveReloadRoundTrip) {
    const Spark::Utf8String prefabPath = Spark::ScenePathResolver::BuildRuntimePath("prefabs", "crate.sparkscene");
    struct stat prefabStat {};
    if (stat(prefabPath.CStr(), &prefabStat) != 0 || !S_ISREG(prefabStat.st_mode)) {
        GTEST_SKIP() << "crate.sparkscene prefab not available";
    }

    std::remove(SmokeScenePath().CStr());

    EditorSmokeHarness harness{};
    harness.SetupUnitCube();
    ASSERT_TRUE(harness.LoadSceneFile("arena.sparkscene"));
    ASSERT_TRUE(harness.PlaceCratePrefab({3.0F, 0.0F, 2.0F}));
    const std::size_t savedEntities = harness.SaveTrackedScene(kSmokeSceneFile);
    ASSERT_GE(savedEntities, 6U);
    EXPECT_TRUE(FileContains(SmokeScenePath().CStr(), "mesh unit_cube \"builtin:unit_cube\""));

    harness.ReloadLikePlayMode(kSmokeSceneFile);
    const Spark::GameObject* crate = FindByName(harness.world, "Crate");
    ASSERT_NE(crate, nullptr);
    const Spark::MeshComponent* mesh = crate->GetComponent<Spark::MeshComponent>();
    ASSERT_NE(mesh, nullptr);
    EXPECT_FLOAT_EQ(mesh->GetAlbedo().x, 0.72F);

    std::remove(SmokeScenePath().CStr());
}

TEST(SceneEditorSmokeTest, GltfPrefabSaveReloadDoesNotDuplicateMeshes) {
    const Spark::Utf8String prefabPath =
            Spark::ScenePathResolver::BuildRuntimePath("prefabs", "imported_DamagedHelmet.sparkscene");
    struct stat prefabStat {};
    if (stat(prefabPath.CStr(), &prefabStat) != 0 || !S_ISREG(prefabStat.st_mode)) {
        GTEST_SKIP() << "imported_DamagedHelmet.sparkscene prefab not available";
    }

    std::remove(SmokeScenePath().CStr());

    EditorSmokeHarness harness{};
    harness.SetupUnitCube();
    ASSERT_TRUE(harness.LoadSceneFile("arena.sparkscene"));
    ASSERT_TRUE(harness.PlaceGltfPrefab("imported_DamagedHelmet.sparkscene", {0.0F, 0.0F, 4.0F}));

    const Spark::GameObject* helmet = FindByName(harness.world, "DamagedHelmet");
    ASSERT_NE(helmet, nullptr);
    const std::size_t objectsBefore = CountSubtreeObjects(*helmet);
    const std::size_t renderablesBefore = CountRenderableComponentsInSubtree(*helmet);
    ASSERT_GT(objectsBefore, 1U);
    ASSERT_GT(renderablesBefore, 0U);

    const std::size_t savedEntities = harness.SaveTrackedScene(kSmokeSceneFile);
    EXPECT_EQ(savedEntities, 6U);
    EXPECT_TRUE(FileContains(SmokeScenePath().CStr(), "gltf_scene"));
    EXPECT_TRUE(FileContains(SmokeScenePath().CStr(), "v2 \""));
    EXPECT_FALSE(FileContains(SmokeScenePath().CStr(), "skinned_mesh"));

    harness.ReloadLikePlayMode(kSmokeSceneFile);

    const Spark::GameObject* helmetAfter = FindByName(harness.world, "DamagedHelmet");
    ASSERT_NE(helmetAfter, nullptr);
    EXPECT_EQ(CountSubtreeObjects(*helmetAfter), objectsBefore);
    EXPECT_EQ(CountRenderableComponentsInSubtree(*helmetAfter), renderablesBefore);

    std::remove(SmokeScenePath().CStr());
}

TEST(SceneEditorSmokeTest, GltfPrefabMaterialOverridesSurviveSaveReload) {
    const Spark::Utf8String prefabPath =
            Spark::ScenePathResolver::BuildRuntimePath("prefabs", "imported_DamagedHelmet.sparkscene");
    struct stat prefabStat {};
    if (stat(prefabPath.CStr(), &prefabStat) != 0 || !S_ISREG(prefabStat.st_mode)) {
        GTEST_SKIP() << "imported_DamagedHelmet.sparkscene prefab not available";
    }

    std::remove(SmokeScenePath().CStr());

    EditorSmokeHarness harness{};
    harness.SetupUnitCube();
    ASSERT_TRUE(harness.LoadSceneFile("arena.sparkscene"));
    ASSERT_TRUE(harness.PlaceGltfPrefab("imported_DamagedHelmet.sparkscene", {1.0F, 0.0F, 4.0F}));

    Spark::GameObject* helmet = FindByName(harness.world, "DamagedHelmet");
    ASSERT_NE(helmet, nullptr);
    Spark::MaterialComponent* editedMaterial = FindFirstMaterialDescendant(*helmet);
    ASSERT_NE(editedMaterial, nullptr);
    editedMaterial->SetMetallic(0.17F);
    editedMaterial->SetRoughness(0.83F);
    editedMaterial->SetEmissive({0.2F, 0.1F, 0.05F}, 2.5F);

    const std::size_t savedEntities = harness.SaveTrackedScene(kSmokeSceneFile);
    ASSERT_EQ(savedEntities, 6U);
    EXPECT_TRUE(FileContains(SmokeScenePath().CStr(), " mat "));

    harness.ReloadLikePlayMode(kSmokeSceneFile);

    const Spark::GameObject* helmetAfter = FindByName(harness.world, "DamagedHelmet");
    ASSERT_NE(helmetAfter, nullptr);
    const Spark::MaterialComponent* restoredMaterial =
            FindFirstMaterialDescendant(const_cast<Spark::GameObject&>(*helmetAfter));
    ASSERT_NE(restoredMaterial, nullptr);
    EXPECT_FLOAT_EQ(restoredMaterial->GetMetallic(), 0.17F);
    EXPECT_FLOAT_EQ(restoredMaterial->GetRoughness(), 0.83F);
    EXPECT_FLOAT_EQ(restoredMaterial->GetEmissiveIntensity(), 2.5F);

    std::remove(SmokeScenePath().CStr());
}

TEST(SceneEditorSmokeTest, LegacyPrefabMeshPathRestoresAsUnitCube) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    Spark::SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = SPARK_ASSETS_DIR;
    applyCtx.assetLoader = &world.GetAssetLoader();

    Spark::ComponentRecord record{};
    record.kind = Spark::Utf8String("mesh");
    record.payload = Spark::Utf8String("unit_cube \"prefabs/barrel.sparkscene\" 0.550000 0.350000 0.220000");

    const Spark::IComponentSnapshotHandler* handler =
            Spark::ComponentSnapshotRegistry::Default().Find(Spark::ComponentKind::Mesh);
    ASSERT_NE(handler, nullptr);
    ASSERT_TRUE(handler->TryRestore(*object, record, world, applyCtx));

    const Spark::MeshComponent* mesh = object->GetComponent<Spark::MeshComponent>();
    ASSERT_NE(mesh, nullptr);
    EXPECT_FLOAT_EQ(mesh->GetAlbedo().x, 0.55F);
    EXPECT_TRUE(mesh->GetMesh());
}

TEST(SceneEditorSmokeTest, PrefabInstantiateOnly) {
    const Spark::Utf8String prefabPath = Spark::ScenePathResolver::BuildRuntimePath("prefabs", "crate.sparkscene");
    struct stat prefabStat {};
    if (stat(prefabPath.CStr(), &prefabStat) != 0 || !S_ISREG(prefabStat.st_mode)) {
        GTEST_SKIP() << "crate.sparkscene prefab not available";
    }

    EditorSmokeHarness harness{};
    harness.SetupUnitCube();
    ASSERT_TRUE(harness.PlaceCratePrefab({2.0F, 0.0F, 1.0F}));
    EXPECT_NE(FindByName(harness.world, "Crate"), nullptr);
}
