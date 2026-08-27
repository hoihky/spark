#include "spark/demo/ModelViewer3DDemo.hpp"

#include "spark/config.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Constants.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/mesh/SkinnedMesh.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/texture/Texture2D.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>

namespace Spark {
namespace {

struct ModelEntry {
    const char* relativePath = nullptr;
    const char* label = nullptr;
    float targetExtentM = 2.5F;
    float yawRadians = 0.0F;
};

constexpr ModelEntry kModels[] = {
        {"/models/DamagedHelmet.glb", "Damaged Helmet", 2.4F, 0.0F},
        {"/models/SheenChair.glb", "Sheen Chair", 2.2F, 0.0F},
        {"/models/CarConcept.glb", "Car Concept", 5.6F, Pi * 0.12F},



        {"/models/ABeautifulGame.glb", "A Beautiful Game", 8.0F, 0.0F},
        {"/models/LightsPunctualLamp.glb", "Punctual Lamp", 2.0F, 0.0F},
        {"/models/ChairDamaskPurplegold.glb", "Damask Chair", 2.2F, 0.0F},
{"/models/ChronographWatch.glb", "Chronograph Watch", 2.2F, 0.0F},

{"/models/Lantern.glb", "Lantern", 2.2F, 0.0F},


};

constexpr std::size_t kModelCount = sizeof(kModels) / sizeof(kModels[0]);

[[nodiscard]] bool TryResolveModelPath(const char* relativePath, char* outPath, const std::size_t outSize) {
    if (relativePath == nullptr || relativePath[0] == '\0' || outPath == nullptr || outSize == 0) {
        return false;
    }
    const char* roots[] = {SPARK_ASSETS_DIR, SPARK_BUILD_ASSETS_DIR, "assets", nullptr};
    for (std::size_t ri = 0; roots[ri] != nullptr; ++ri) {
        std::snprintf(outPath, outSize, "%s%s", roots[ri], relativePath);
        FILE* f = std::fopen(outPath, "rb");
        if (f != nullptr) {
            std::fclose(f);
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool TryLoadGltfEnvHdr(GameWorld& w, SharedPtr<Texture2D>& outTex) {
    Texture2D decoded;
    const char* relativePath = "/textures/sky/venice_sunset_1k.hdr";
    const char* roots[] = {SPARK_ASSETS_DIR, SPARK_BUILD_ASSETS_DIR, "assets", nullptr};
    char pathBuf[768]{};
    for (std::size_t ri = 0; roots[ri] != nullptr; ++ri) {
        std::snprintf(pathBuf, sizeof(pathBuf), "%s%s", roots[ri], relativePath);
        if (Texture2D::TryLoadFromFile(pathBuf, decoded)) {
            outTex = MakeShared<Texture2D>(MoveTemp(decoded));
            w.RegisterTexture(outTex, "spark/demo/model_viewer_env_hdr");
            return true;
        }
    }
    return false;
}

void VisitObjectTree(GameObject* object, const std::function<void(GameObject&)>& visitor) {
    if (object == nullptr) {
        return;
    }
    visitor(*object);
    const Array<GameObject*>& children = object->GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        VisitObjectTree(children[i], visitor);
    }
}

void ExpandBoundsPoint(const Vector3& point, Vector3& bmin, Vector3& bmax, bool& any) noexcept {
    if (!any) {
        bmin = point;
        bmax = point;
        any = true;
        return;
    }
    bmin.x = std::min(bmin.x, point.x);
    bmin.y = std::min(bmin.y, point.y);
    bmin.z = std::min(bmin.z, point.z);
    bmax.x = std::max(bmax.x, point.x);
    bmax.y = std::max(bmax.y, point.y);
    bmax.z = std::max(bmax.z, point.z);
}

void ExpandBoundsWithLocalBox(
        const Matrix4& worldMatrix,
        const Vector3& localMin,
        const Vector3& localMax,
        Vector3& bmin,
        Vector3& bmax,
        bool& any) noexcept {
    const Vector3 corners[8] = {
            {localMin.x, localMin.y, localMin.z},
            {localMax.x, localMin.y, localMin.z},
            {localMin.x, localMax.y, localMin.z},
            {localMax.x, localMax.y, localMin.z},
            {localMin.x, localMin.y, localMax.z},
            {localMax.x, localMin.y, localMax.z},
            {localMin.x, localMax.y, localMax.z},
            {localMax.x, localMax.y, localMax.z},
    };
    for (const Vector3& corner : corners) {
        ExpandBoundsPoint(worldMatrix.TransformPoint(corner), bmin, bmax, any);
    }
}

[[nodiscard]] bool TryComputeSubtreeWorldBounds(GameObject& root, Vector3& bmin, Vector3& bmax) {
    bool any = false;
    VisitObjectTree(&root, [&](GameObject& object) {
        const Matrix4 worldMatrix = object.GetWorldMatrix();
        if (const MeshComponent* meshComp = object.GetComponent<MeshComponent>()) {
            if (const SharedPtr<Mesh>& mesh = meshComp->GetMesh()) {
                Vector3 localMin{};
                Vector3 localMax{};
                if (mesh->TryComputeAxisAlignedBounds(localMin, localMax)) {
                    ExpandBoundsWithLocalBox(worldMatrix, localMin, localMax, bmin, bmax, any);
                }
            }
        }
        if (const SkinnedMeshComponent* skinned = object.GetComponent<SkinnedMeshComponent>()) {
            if (const SharedPtr<SkinnedMesh>& mesh = skinned->GetMesh()) {
                const Array<SkinnedMesh::Vertex>& verts = mesh->GetVertices();
                for (std::size_t vi = 0; vi < verts.GetSize(); ++vi) {
                    ExpandBoundsPoint(worldMatrix.TransformPoint(verts[vi].position), bmin, bmax, any);
                }
            }
        }
    });
    return any;
}

void FitPivotToGround(GameObject& pivot, const float targetExtentM) {
    TransformComponent* tr = pivot.GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }
    tr->SetUniformScale(1.0F);
    tr->SetTranslation(Vector3::Zero);

    Vector3 bmin{};
    Vector3 bmax{};
    if (!TryComputeSubtreeWorldBounds(pivot, bmin, bmax)) {
        return;
    }
    const float maxExt = std::max({bmax.x - bmin.x, bmax.y - bmin.y, bmax.z - bmin.z});
    float uniformScale = 1.0F;
    if (maxExt > 1.0e-4F) {
        uniformScale = targetExtentM / maxExt;
    }
    constexpr float kGroundClearance = 0.03F;
    tr->SetUniformScale(uniformScale);
    tr->SetTranslation({0.0F, -bmin.y * uniformScale + kGroundClearance, 0.0F});
}

}  // namespace

void ModelViewer3DDemo::FrameCamera(const float targetExtentM) noexcept {
    const float dist = targetExtentM * 1.75F + 1.8F;
    camera.position = {dist * 0.5F, targetExtentM * 0.42F + 0.65F, dist};
    camera.SnapLookAt({0.0F, targetExtentM * 0.32F, 0.0F});
}

void ModelViewer3DDemo::RefreshHud() noexcept {
    char detail[192]{};
    if (currentIndex < kModelCount) {
        const ModelEntry& spec = kModels[currentIndex];
        std::snprintf(
                detail,
                sizeof(detail),
                "%zu / %zu — %s%s",
                currentIndex + 1U,
                kModelCount,
                spec.label,
                currentLoadOk ? "" : " (failed to load)");
    } else {
        std::snprintf(detail, sizeof(detail), "No models configured");
    }
    helpHud.SetDetail(detail);
}

void ModelViewer3DDemo::ShowModel(GameWorld& w, const std::size_t index) {
    if (modelPivot != nullptr) {
        w.DestroyGameObject(modelPivot);
        modelPivot = nullptr;
    }

    currentIndex = index % kModelCount;
    currentLoadOk = false;
    const ModelEntry& spec = kModels[currentIndex];

    char pathBuf[768]{};
    if (!TryResolveModelPath(spec.relativePath, pathBuf, sizeof(pathBuf))) {
        RefreshHud();
        return;
    }

    modelPivot = w.CreateGameObject();
    modelPivot->GetName() = Utf8String(spec.label);
    TransformComponent* pivotTr = modelPivot->AddComponent<TransformComponent>();
    pivotTr->SetRotation(Quaternion::FromAxisAngle(Vector3::UnitY, spec.yawRadians));

    bool placed = false;
    if (GltfAssetBinder::BindFromPath(*modelPivot, pathBuf, SceneMeshSlot::Custom, Vector3::One)) {
        placed = true;
    } else {
        const SkinnedGltfAsset skinned = w.LoadSkinnedGltf(pathBuf);
        if (skinned.mesh) {
            GltfAssetBinder::BindSkinnedMesh(*modelPivot, skinned, Vector3::One, pathBuf);
            if (skinned.skeleton && skinned.skeleton->GetClipCount() > 0U) {
                modelPivot->AddComponent<AnimatorComponent>(
                        skinned.skeleton, skinned.walkClipIndex, 1.0F);
            }
            placed = true;
        } else {
            const GltfAsset rigid = w.LoadGltf(pathBuf);
            if (rigid.mesh) {
                GltfAssetBinder::BindRigidMesh(*modelPivot, rigid, SceneMeshSlot::Custom, Vector3::One, pathBuf);
                placed = true;
            }
        }
    }

    if (placed) {
        FitPivotToGround(*modelPivot, spec.targetExtentM);
        FrameCamera(spec.targetExtentM);
        currentLoadOk = true;
    }

    RefreshHud();
}

void ModelViewer3DDemo::AdvanceModel(GameWorld& w, const int delta) {
    if (kModelCount == 0) {
        return;
    }
    int next = static_cast<int>(currentIndex) + delta;
    next %= static_cast<int>(kModelCount);
    if (next < 0) {
        next += static_cast<int>(kModelCount);
    }
    ShowModel(w, static_cast<std::size_t>(next));
}

void ModelViewer3DDemo::Load(GameWorld& w, IEngineContext& context) {
    sceneRoots.Clear();
    modelPivot = nullptr;
    currentIndex = 0;
    currentLoadOk = false;
    envHdrLoaded = false;
    envEquirectTex.Reset();

    envHdrLoaded = TryLoadGltfEnvHdr(w, envEquirectTex);

    skyMesh = MakeShared<Mesh>(Utf8String("ModelViewerSky"));
    *skyMesh = Mesh::CreateSkySphere(1.0F, 20, 40);
    GameObject* sky = w.CreateGameObject();
    sky->GetName() = Utf8String("ModelViewerSky");
    if (TransformComponent* skyTr = sky->AddComponent<TransformComponent>()) {
        skyTr->SetUniformScale(120.0F);
    }
    SkyComponent* skyComp = sky->AddComponent<SkyComponent>(SceneSkyMode::Dome);
    MeshComponent* skyMeshComp = sky->AddComponent<MeshComponent>(skyMesh, SceneMeshSlot::Custom, Vector3::One);
    MaterialComponent* skyMat = sky->AddComponent<MaterialComponent>();
    if (envHdrLoaded && envEquirectTex) {
        skyComp->SetTint(Vector3::One);
        skyMat->SetBaseColorTexture(envEquirectTex);
        skyMat->SetTint(Vector3::One);
    } else {
        constexpr Vector3 kFallbackGrey{0.52F, 0.54F, 0.58F};
        skyComp->SetTint(kFallbackGrey);
        skyMeshComp->SetAlbedo(kFallbackGrey);
    }
    sceneRoots.PushBack(sky);

    groundMesh = MakeShared<Mesh>(Utf8String("ModelViewerGround"));
    *groundMesh = Mesh::CreateGroundPlane(kSceneGroundHalfExtent);
    w.RegisterMesh(groundMesh, "spark/demo/model_viewer_ground");

    GameObject* ground = w.CreateGameObject();
    ground->GetName() = Utf8String("Ground");
    ground->AddComponent<MeshComponent>(groundMesh, SceneMeshSlot::GroundPlane, Vector3{0.46F, 0.48F, 0.50F});
    sceneRoots.PushBack(ground);

    helpHud.Mount(w, "3D model viewer");
    helpHud.SetControlHints("TAB next model | V paint variant | ESC menu | F1 fly");
    ShowModel(w, 0);

    context.GetInput().SetCursorCaptured(true);
}

void ModelViewer3DDemo::Unload(GameWorld& w) {
    helpHud.Unmount(w);
    if (modelPivot != nullptr) {
        w.DestroyGameObject(modelPivot);
        modelPivot = nullptr;
    }
    for (std::size_t i = 0; i < sceneRoots.GetSize(); ++i) {
        if (sceneRoots[i] != nullptr) {
            w.DestroyGameObject(sceneRoots[i]);
        }
    }
    sceneRoots.Clear();
    groundMesh.Reset();
    skyMesh.Reset();
    envEquirectTex.Reset();
    envHdrLoaded = false;
    currentLoadOk = false;
}

void ModelViewer3DDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world) {
    IInput& in = context.GetInput();
    if (in.IsKeyPressedThisFrame(GLFW_KEY_TAB)) {
        AdvanceModel(world, 1);
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_V)) {
        for (std::size_t ri = 0; ri < sceneRoots.GetSize(); ++ri) {
            MultiMaterialComponent::CycleVariantsOnObjectTree(sceneRoots[ri]);
        }
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        in.SetCursorCaptured(!in.IsCursorCaptured());
    }
    if (in.IsCursorCaptured()) {
        if (timing.frameIndex > 0) {
            camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        }
        camera.ProcessMovement(in, timing.deltaTimeSeconds);
    }
    helpHud.Update(timing, context);
}

void ModelViewer3DDemo::Render(Scene& scene, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(45.0F), aspect, 0.08F, 120.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    scene.SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);
    scene.ApplySpatialPolicyFromFirstMatchingObject();

    SceneRenderParams params{};
    params.iblEnvironmentLayer = -1;
    params.iblIntensity = 0.72F;
    params.iblEnabled = true;
    params.iblUseHdrSkyEnvironment = true;
    params.directionalShadowsEnabled = true;
    params.ssaoEnabled = false;
    FillStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Vector3{0.35F, 0.88F, 0.32F}.Normalized(),
            {1.0F, 0.96F, 0.90F},
            0.92F,
            {0.10F, 0.11F, 0.13F},
            false,
            {},
            {},
            0.0F,
            params,
            SceneSpriteSortMode::SortOrderOnly,
            &scene);

    scene.ForEachTextOverlay([&params](const TextOverlayComponent& tc) {
        ScreenTextDraw d{};
        d.text = tc.GetText();
        d.x = tc.GetScreenX();
        d.y = tc.GetScreenY();
        d.sizePixels = tc.GetFontSizePixels();
        d.color = tc.GetColor();
        d.alpha = tc.GetAlpha();
        d.paintOrder = params.NextUiPaintOrder();
        params.screenTexts.PushBack(MoveTemp(d));
    });

    helpHud.PatchSceneRenderParams(params, world);
    context.SetSceneRenderParams(params);
}

}  // namespace Spark
