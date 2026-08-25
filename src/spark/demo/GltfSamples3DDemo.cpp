#include "spark/demo/GltfSamples3DDemo.hpp"

#include "spark/config.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/texture/Texture2D.hpp"

#include <cmath>
#include <cstdio>
#include <format>
#include <string>

namespace Spark {
namespace {

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

[[nodiscard]] bool TryLoadStudioHdr(GameWorld& w, SharedPtr<Texture2D>& outTex) {
    Texture2D decoded;
    if (Texture2D::TryLoadFromFile(SPARK_STUDIO_HDR_PATH, decoded)) {
        outTex = MakeShared<Texture2D>(MoveTemp(decoded));
        w.RegisterTexture(outTex, "spark/demo/gltf_studio_hdr");
        return true;
    }
    const char* relativePath = "/textures/sky/studio_small_08_1k.hdr";
    const char* roots[] = {SPARK_ASSETS_DIR, SPARK_BUILD_ASSETS_DIR, "assets", nullptr};
    char pathBuf[768]{};
    for (std::size_t ri = 0; roots[ri] != nullptr; ++ri) {
        std::snprintf(pathBuf, sizeof(pathBuf), "%s%s", roots[ri], relativePath);
        if (Texture2D::TryLoadFromFile(pathBuf, decoded)) {
            outTex = MakeShared<Texture2D>(MoveTemp(decoded));
            w.RegisterTexture(outTex, "spark/demo/gltf_studio_hdr");
            return true;
        }
    }
    return false;
}

}  // namespace

bool GltfSamples3DDemo::TryPlaceRigidGltf(
        GameWorld& w,
        const char* relativePath,
        const Vector3& pos,
        const float yawRadians,
        const float targetMaxExtentM) {
    char pathBuf[768]{};
    if (!TryResolveModelPath(relativePath, pathBuf, sizeof(pathBuf))) {
        return false;
    }

    const GltfAsset asset = w.LoadGltf(pathBuf);
    if (!asset.mesh) {
        return false;
    }

    Vector3 bmin{};
    Vector3 bmax{};
    float uniformScale = 1.0F;
    if (asset.mesh->TryComputeAxisAlignedBounds(bmin, bmax)) {
        const float maxExt = std::max({bmax.x - bmin.x, bmax.y - bmin.y, bmax.z - bmin.z});
        if (maxExt > 1.0e-4F) {
            uniformScale = targetMaxExtentM / maxExt;
        }
    }

    GameObject* go = w.CreateGameObject();
    const char* slash = std::strrchr(relativePath, '/');
    go->GetName() = Utf8String(slash != nullptr ? slash + 1 : relativePath);

    TransformComponent* tr = go->AddComponent<TransformComponent>();
    tr->SetUniformScale(uniformScale);
    constexpr float kGroundClearance = 0.05F;
    tr->SetTranslation({pos.x, -bmin.y * uniformScale + kGroundClearance, pos.z});
    tr->SetRotation(Quaternion::FromAxisAngle(Vector3::UnitY, yawRadians));

    GltfAssetBinder::BindRigidMesh(*go, asset, SceneMeshSlot::Custom, Vector3::One, pathBuf);
    if (MaterialComponent* mat = go->GetComponent<MaterialComponent>()) {
        if (!asset.materials.IsEmpty()) {
            ApplyGltfMaterialDesc(*mat, asset.materials[0]);
        } else if (asset.material.HasAnyTexture()) {
            ApplyGltfMaterialDesc(*mat, asset.material);
        }
    }
    roots.PushBack(go);
    return true;
}

void GltfSamples3DDemo::Load(GameWorld& w, IEngineContext& /*context*/) {
    helmetLoaded = false;
    envHdrLoaded = false;
    envEquirectTex.Reset();

    envHdrLoaded = TryLoadStudioHdr(w, envEquirectTex);

    skyMesh = MakeShared<Mesh>(Utf8String("GltfSamplesSky"));
    *skyMesh = Mesh::CreateSkySphere(1.0F, 20, 40);
    GameObject* sky = w.CreateGameObject();
    sky->GetName() = Utf8String("StudioSky");
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
        constexpr Vector3 kFallbackGrey{0.54F, 0.56F, 0.58F};
        skyComp->SetTint(kFallbackGrey);
        skyMeshComp->SetAlbedo(kFallbackGrey);
    }
    roots.PushBack(sky);

    groundMesh = MakeShared<Mesh>(Utf8String("GltfSamplesGround"));
    *groundMesh = Mesh::CreateGroundPlane(kSceneGroundHalfExtent);
    w.RegisterMesh(groundMesh, "spark/demo/gltf_samples_ground");

    GameObject* ground = w.CreateGameObject();
    ground->GetName() = Utf8String("Ground");
    ground->AddComponent<MeshComponent>(groundMesh, SceneMeshSlot::GroundPlane, Vector3{0.48F, 0.50F, 0.52F});
    roots.PushBack(ground);

    helmetLoaded = TryPlaceRigidGltf(w, "/models/DamagedHelmet.glb", {0.0F, 0.0F, 0.0F}, Pi, 2.4F);

    constexpr Vector3 kHelmetTarget{0.0F, 1.0F, 0.0F};

    helpHud = w.CreateGameObject();
    helpHud->GetName() = Utf8String("GltfSamplesHelp");
    helpText = helpHud->AddComponent<TextOverlayComponent>();
    helpText->SetScreenPosition(DemoHud::kScreenMargin, DemoHud::kScreenMargin);
    DemoHud::Apply(*helpText);

    std::string msg = "glTF sample — DamagedHelmet.glb\n";
    msg += std::format(
            "  Helmet: {} · IBL: {}\n"
            "  F1 mouse lock · WASD fly · ESC menu",
            helmetLoaded ? "loaded" : "missing",
            envHdrLoaded ? "studio HDR" : "missing (procedural)");
    if (!helmetLoaded) {
        msg += "\n  Expected path: assets/models/DamagedHelmet.glb";
    }
    if (!envHdrLoaded) {
        msg += "\n  Expected HDR: assets/textures/sky/studio_small_08_1k.hdr";
    }
    helpText->SetText(Utf8String(msg.c_str()));
    roots.PushBack(helpHud);

    camera.position = {0.0F, 1.35F, 4.8F};
    camera.SnapLookAt(kHelmetTarget);
}

void GltfSamples3DDemo::Unload(GameWorld& w) {
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    groundMesh.Reset();
    skyMesh.Reset();
    envEquirectTex.Reset();
    helpHud = nullptr;
    helpText = nullptr;
    helmetLoaded = false;
    envHdrLoaded = false;
}

void GltfSamples3DDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& /*world*/) {
    IInput& in = context.GetInput();
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        in.SetCursorCaptured(!in.IsCursorCaptured());
    }
    if (in.IsCursorCaptured()) {
        if (timing.frameIndex > 0) {
            camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        }
        camera.ProcessMovement(in, timing.deltaTimeSeconds);
    }
}

void GltfSamples3DDemo::Render(Scene& scene, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(45.0F), aspect, 0.08F, 80.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    scene.SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);
    scene.ApplySpatialPolicyFromFirstMatchingObject();

    SceneRenderParams params{};
    params.iblEnvironmentLayer = -1;
    params.iblIntensity = 1.0F;
    params.iblEnabled = true;
    params.directionalShadowsEnabled = true;
    params.ssaoEnabled = false;
    FillStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Vector3{0.35F, 0.88F, 0.32F}.Normalized(),
            {1.0F, 0.96F, 0.90F},
            0.90F,
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

    context.SetSceneRenderParams(params);
}

}  // namespace Spark
