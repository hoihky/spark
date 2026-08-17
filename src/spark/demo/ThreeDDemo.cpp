#include "spark/demo/ThreeDDemo.hpp"
#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/scene/GltfAssetBindings.hpp"

#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/rendering/BillboardComponent.hpp"
#include "spark/ecs/components/rendering/DecalProjectorComponent.hpp"
#include "spark/ecs/components/world/SceneSpatialPolicyComponent.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/scene/detail/SceneSubmitDetail.hpp"

namespace Spark {

void ThreeDDemo::RequestGltfAssets(Spark::GameWorld& world) noexcept {
    Spark::Utf8String helmetPath(SPARK_ASSETS_DIR);
    helmetPath.AppendUtf8("/models/DamagedHelmet.glb");
    world.RequestGltf(helmetPath.CStr());

    Spark::Utf8String sheenChairPath(SPARK_ASSETS_DIR);
    sheenChairPath.AppendUtf8("/models/SheenChair.glb");
    world.RequestGltf(sheenChairPath.CStr());

    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    world.RequestSkinnedGltf(foxPath.CStr());
}

void ThreeDDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        loadedWorld = &w;
        roots.Clear();
        roots.Reserve(48);

        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("UnitCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        w.RegisterMesh(unitCubeAsset, "spark/demo/unit_cube");

        groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("Ground"));
        *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent);
        w.RegisterMesh(groundAsset, "spark/demo/ground");

        checkerTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("DemoGroundTex"));
        if (!DemoAssets::TryLoadGroundDirtTexture(*checkerTex)) {
            *checkerTex = Spark::Texture2D::CreateCheckerboard(
                    256,
                    32,
                    Spark::Vector3{0.92F, 0.86F, 0.72F},
                    Spark::Vector3{0.25F, 0.35F, 0.55F});
            checkerTex->GetName() = Spark::Utf8String("DemoCheckerFallback");
        }
        w.RegisterTexture(checkerTex, "spark/demo/checker");

        brickTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("BrickPattern"));
        if (!DemoAssets::TryLoadBrickTexture(*brickTex)) {
            *brickTex = Spark::Texture2D::CreateBrickPattern(256, 256);
            w.RegisterTexture(brickTex, "spark/demo/brick_fallback");
        } else {
            w.RegisterTexture(brickTex, "spark/demo/bricks");
        }

        groundObject = w.CreateGameObject();
        groundObject->GetName() = Spark::Utf8String("Ground");
        groundObject->AddComponent<Spark::TransformComponent>();
        groundObject->AddComponent<Spark::MeshComponent>(
                groundAsset, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3{0.58F, 0.6F, 0.64F});
        groundObject->AddComponent<Spark::SceneSpatialPolicyComponent>(
                Spark::ScenePartitionKind::BoundingVolumeHierarchy);
        roots.PushBack(groundObject);

        cubeObject = w.CreateGameObject();
        cubeObject->GetName() = Spark::Utf8String("Cube");
        {
            Spark::TransformComponent* tr = cubeObject->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({0.0F, kCubeScale, 0.0F});
            tr->SetUniformScale(kCubeScale);
        }
        cubeObject->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.75F, 0.42F, 0.32F});
        constexpr float kCollideRadius = kCubeScale * 1.75F;
        cubeObject->AddComponent<Spark::CollisionComponent>(kCollideRadius, Spark::Vector3::Zero);
        if (Spark::MaterialComponent* emMat = cubeObject->AddComponent<Spark::MaterialComponent>()) {
            emMat->SetEmissive({0.95F, 0.48F, 0.1F}, 2.4F);
            emMat->SetRoughness(0.35F);
        }
        roots.PushBack(cubeObject);

        Spark::SharedPtr<Spark::Mesh> billboardMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("DemoBillboard"));
        *billboardMesh = Spark::Mesh::CreateSkyBillboardPlane(0.55F, 0.55F);
        Spark::GameObject* billboardMarker = w.CreateGameObject();
        billboardMarker->GetName() = Spark::Utf8String("BillboardMarker");
        {
            Spark::TransformComponent* tr = billboardMarker->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({0.0F, 1.65F, 3.2F});
        }
        billboardMarker->AddComponent<Spark::MeshComponent>(
                billboardMesh, Spark::SceneMeshSlot::Custom, Spark::Vector3{0.2F, 0.95F, 0.35F});
        if (Spark::MaterialComponent* bm = billboardMarker->AddComponent<Spark::MaterialComponent>()) {
            bm->SetEmissive({0.35F, 1.0F, 0.45F}, 3.5F);
            bm->SetRoughness(0.2F);
        }
        Spark::BillboardComponent* billboard = billboardMarker->AddComponent<Spark::BillboardComponent>();
        billboard->SetMode(Spark::BillboardMode::YAxisLocked);
        roots.PushBack(billboardMarker);

        Spark::GameObject* decalGo = w.CreateGameObject();
        decalGo->GetName() = Spark::Utf8String("GroundDecal");
        {
            Spark::TransformComponent* tr = decalGo->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({0.0F, 0.02F, 0.0F});
            tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitX, Spark::HalfPi));
        }
        Spark::DecalProjectorComponent* decal = decalGo->AddComponent<Spark::DecalProjectorComponent>();
        decal->SetTexture(checkerTex);
        decal->SetSize({2.4F, 2.4F, 0.35F});
        decal->SetOpacity(0.72F);
        roots.PushBack(decalGo);

        Spark::GameObject* texturedCube = w.CreateGameObject();
        texturedCube->GetName() = Spark::Utf8String("TexturedCube");
        {
            Spark::TransformComponent* tr = texturedCube->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({3.0F, kCubeScale, 0.0F});
            tr->SetUniformScale(kCubeScale);
        }
        texturedCube->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{1.0F, 1.0F, 1.0F});
        if (Spark::MaterialComponent* chk = texturedCube->AddComponent<Spark::MaterialComponent>(
                    checkerTex, Spark::Vector3::One)) {
            chk->SetMetallic(0.04F);
            chk->SetRoughness(0.22F);
        }
        roots.PushBack(texturedCube);

        Spark::GameObject* brickCube = w.CreateGameObject();
        brickCube->GetName() = Spark::Utf8String("BrickCube");
        {
            Spark::TransformComponent* tr = brickCube->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({-3.0F, kCubeScale, 0.0F});
            tr->SetUniformScale(kCubeScale);
        }
        brickCube->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{1.0F, 1.0F, 1.0F});
        if (Spark::MaterialComponent* br = brickCube->AddComponent<Spark::MaterialComponent>(
                    brickTex, Spark::Vector3::One)) {
            br->SetMetallic(0.0F);
            br->SetRoughness(0.88F);
        }
        roots.PushBack(brickCube);

        RequestGltfAssets(w);

        Spark::Utf8String helmetPath(SPARK_ASSETS_DIR);
        helmetPath.AppendUtf8("/models/DamagedHelmet.glb");
        pendingGltfLoads.PushBack(PendingGltfLoad{helmetPath, PendingGltfKind::Hero, false});

        Spark::Utf8String sheenChairPath(SPARK_ASSETS_DIR);
        sheenChairPath.AppendUtf8("/models/SheenChair.glb");
        pendingGltfLoads.PushBack(PendingGltfLoad{sheenChairPath, PendingGltfKind::Chair, false});

        Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
        foxPath.AppendUtf8("/models/Fox.glb");
        pendingGltfLoads.PushBack(PendingGltfLoad{foxPath, PendingGltfKind::Fox, false});

        PollPendingGltfLoads(w);

        auto addPointLight = [this, &w](Spark::Vector3 pos, Spark::Vector3 color, float intensity, float range) {
            Spark::GameObject* light = w.CreateGameObject();
            light->GetName() = Spark::Utf8String("PointLight");
            Spark::TransformComponent* tr = light->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation(pos);
            light->AddComponent<Spark::PointLightComponent>(color, intensity, range)->SetCastsShadow(true);
            roots.PushBack(light);
        };
        addPointLight({2.2F, 3.8F, 5.5F}, {0.35F, 0.72F, 1.0F}, 4.5F, 19.0F);
        addPointLight({-6.5F, 2.2F, -1.5F}, {1.0F, 0.32F, 0.18F}, 3.8F, 15.0F);
        addPointLight({0.0F, 5.8F, -4.2F}, {0.85F, 0.92F, 0.45F}, 3.0F, 23.0F);

        context.GetInput().SetCursorCaptured(true);
        camera.position = {10.5F, 6.5F, 16.0F};
        camera.SnapLookAt({1.1F, 1.15F, 2.0F});

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("FpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*fpsText);
        fpsText->SetText(Spark::Utf8String("..."));
        roots.PushBack(fpsHudObject);
    }

void ThreeDDemo::Unload(Spark::GameWorld& w)
{
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        groundObject = nullptr;
        cubeObject = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
        pendingGltfLoads.Clear();
        loadedWorld = nullptr;
    }

void ThreeDDemo::SpawnHero(
        Spark::GameWorld& w,
        const Spark::GltfAsset& asset,
        const Spark::Utf8String& path) {
    bool usedHelmetGltf = static_cast<bool>(asset.mesh);
    if (usedHelmetGltf) {
        heroMeshAsset = asset.mesh;
    } else {
        heroMeshAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SimpleCar"));
        *heroMeshAsset = Spark::Mesh::CreateSimpleCar();
        w.RegisterMesh(heroMeshAsset, "spark/demo/simple_car");
        std::println(
                std::cerr,
                "Spark: DamagedHelmet.glb not loaded from {} — using procedural SimpleCar fallback",
                path.CStr());
    }

    Spark::GameObject* heroObject = w.CreateGameObject();
    heroObject->GetName() = Spark::Utf8String(usedHelmetGltf ? "DamagedHelmet" : "SimpleCar");
    Spark::Vector3 heroMin{};
    Spark::Vector3 heroMax{};
    float heroUniformScale = usedHelmetGltf ? 2.8F : 1.35F;
    if (heroMeshAsset->TryComputeAxisAlignedBounds(heroMin, heroMax)) {
        const float dx = heroMax.x - heroMin.x;
        const float dy = heroMax.y - heroMin.y;
        const float dz = heroMax.z - heroMin.z;
        const float maxExt = std::max({dx, dy, dz});
        if (maxExt > 1.0e-4F) {
            heroUniformScale = 2.4F / maxExt;
        }
    }
    {
        Spark::TransformComponent* tr = heroObject->AddComponent<Spark::TransformComponent>();
        tr->SetUniformScale(heroUniformScale);
        constexpr float kGroundClearance = 0.12F;
        const float yOnGround = -heroMin.y * heroUniformScale + kGroundClearance;
        tr->SetTranslation({6.0F, yOnGround, 0.0F});
        if (usedHelmetGltf) {
            tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitY, Spark::Pi));
        }
    }
    Spark::GltfAssetBinder::BindRigidMesh(
            *heroObject,
            usedHelmetGltf ? asset : Spark::GltfAsset{.mesh = heroMeshAsset},
            Spark::SceneMeshSlot::Custom,
            Spark::Vector3{usedHelmetGltf ? 1.0F : 0.92F, usedHelmetGltf ? 1.0F : 0.18F,
                           usedHelmetGltf ? 1.0F : 0.12F});
    std::println(
            std::cerr,
            "Spark: {} — {} vertices, {} indices, {} submeshes",
            usedHelmetGltf ? "Khronos DamagedHelmet" : "procedural SimpleCar",
            heroMeshAsset->GetVertices().GetSize(),
            heroMeshAsset->GetIndices().GetSize(),
            heroMeshAsset->GetSubmeshes().GetSize());
    roots.PushBack(heroObject);
}

void ThreeDDemo::SpawnChair(Spark::GameWorld& w, const Spark::GltfAsset& asset) {
    if (!asset.mesh) {
        return;
    }
    chairMeshAsset = asset.mesh;
    Spark::GameObject* chairObject = w.CreateGameObject();
    chairObject->GetName() = Spark::Utf8String("SheenChair");
    Spark::Vector3 chairMin{};
    Spark::Vector3 chairMax{};
    float chairUniformScale = 1.9F;
    if (chairMeshAsset->TryComputeAxisAlignedBounds(chairMin, chairMax)) {
        const float dx = chairMax.x - chairMin.x;
        const float dy = chairMax.y - chairMin.y;
        const float dz = chairMax.z - chairMin.z;
        const float maxExt = std::max({dx, dy, dz});
        if (maxExt > 1.0e-4F) {
            chairUniformScale = 2.15F / maxExt;
        }
    }
    {
        Spark::TransformComponent* tr = chairObject->AddComponent<Spark::TransformComponent>();
        tr->SetUniformScale(chairUniformScale);
        constexpr float kGroundClearance = 0.12F;
        const float yOnGround = -chairMin.y * chairUniformScale + kGroundClearance;
        tr->SetTranslation({-4.25F, yOnGround, 4.0F});
        tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitY, Spark::Pi * 0.35F));
    }
    Spark::GltfAssetBinder::BindRigidMesh(
            *chairObject, asset, Spark::SceneMeshSlot::Custom, Spark::Vector3{1.0F, 1.0F, 1.0F});
    std::println(
            std::cerr,
            "Spark: Khronos SheenChair — {} vertices, {} indices, {} submeshes",
            chairMeshAsset->GetVertices().GetSize(),
            chairMeshAsset->GetIndices().GetSize(),
            chairMeshAsset->GetSubmeshes().GetSize());
    roots.PushBack(chairObject);
}

void ThreeDDemo::SpawnFox(Spark::GameWorld& w, const Spark::SkinnedGltfAsset& asset) {
    if (!asset.mesh || !asset.skeleton) {
        return;
    }
    Spark::GameObject* foxObject = w.CreateGameObject();
    foxObject->GetName() = Spark::Utf8String("FoxWalk");
    {
        Spark::TransformComponent* tr = foxObject->AddComponent<Spark::TransformComponent>();
        tr->SetUniformScale(0.032F);
        tr->SetTranslation({1.8F, 0.0F, 7.25F});
        tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitY, -Spark::Pi * 0.5F));
    }
    foxObject->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);
    Spark::Character3DAnimFsmComponent* foxFsm = foxObject->AddComponent<Spark::Character3DAnimFsmComponent>();
    foxObject->AddComponent<Spark::AnimatorComponent>(asset.skeleton, asset.walkClipIndex, 1.2F);
    foxFsm->ConfigureLocomotionFromSkeleton(*asset.skeleton, asset.walkClipIndex);
    foxFsm->SetWalkSpeedThreshold(0.25F);
    Spark::GltfAssetBinder::BindSkinnedMesh(*foxObject, asset);
    std::println(
            std::cerr,
            "Spark: Khronos Fox — walk clip {}, {} joints, {} skinned verts, {} submeshes",
            asset.walkClipIndex,
            asset.skeleton->GetJointCount(),
            asset.mesh->GetVertices().GetSize(),
            asset.mesh->GetSubmeshes().GetSize());
    roots.PushBack(foxObject);
}

void ThreeDDemo::PollPendingGltfLoads(Spark::GameWorld& w) {
    for (std::size_t i = 0; i < pendingGltfLoads.GetSize(); ++i) {
        PendingGltfLoad& pending = pendingGltfLoads[i];
        if (pending.spawned) {
            continue;
        }
        if (pending.kind == PendingGltfKind::Fox) {
            Spark::SkinnedGltfAsset skinned{};
            if (!w.TryGetCachedSkinnedGltf(pending.path.CStr(), skinned)) {
                if (w.GetAssetLoadState(pending.path.CStr(), Spark::AssetLoadJobKind::SkinnedGltf) ==
                    Spark::AssetLoadState::Failed) {
                    std::println(
                            std::cerr,
                            "Spark: Fox.glb failed to load from {}",
                            pending.path.CStr());
                    pending.spawned = true;
                }
                continue;
            }
            SpawnFox(w, skinned);
            pending.spawned = true;
            continue;
        }

        Spark::GltfAsset asset{};
        if (!w.TryGetCachedGltf(pending.path.CStr(), asset)) {
            if (w.GetAssetLoadState(pending.path.CStr(), Spark::AssetLoadJobKind::Gltf) ==
                Spark::AssetLoadState::Failed) {
                if (pending.kind == PendingGltfKind::Hero) {
                    SpawnHero(w, {}, pending.path);
                } else {
                    std::println(
                            std::cerr,
                            "Spark: glTF failed to load from {}",
                            pending.path.CStr());
                }
                pending.spawned = true;
            }
            continue;
        }
        if (pending.kind == PendingGltfKind::Hero) {
            SpawnHero(w, asset, pending.path);
        } else {
            SpawnChair(w, asset);
        }
        pending.spawned = true;
    }
}

void ThreeDDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        if (loadedWorld != nullptr) {
            PollPendingGltfLoads(*loadedWorld);
        }

        Spark::IInput& in = context.GetInput();
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (in.IsCursorCaptured()) {
            if (timing.frameIndex > 0) {
                camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
            }
            camera.ProcessMovement(in, timing.deltaTimeSeconds);
        }

        cubeYawRadians =
                std::fmod(cubeYawRadians + timing.deltaTimeSeconds * kCubeSpinRadPerSec, Spark::TwoPi);
        if (Spark::TransformComponent* tr = cubeObject->GetComponent<Spark::TransformComponent>()) {
            tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitY, cubeYawRadians));
        }

        if (fpsText != nullptr) {
            const float dt = timing.deltaTimeSeconds;
            const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            fpsText->SetText(Spark::Utf8String(
                    std::format("{:.0f} FPS — BVH policy · Billboard · DecalProjector",
                                static_cast<double>(fpsSmoothed))
                            .c_str()));
        }
    }

void ThreeDDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

        const Spark::Matrix4 proj =
                Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 400.0F);
        const Spark::Matrix4 view = camera.ViewMatrix();
        const Spark::Matrix4 viewProj = proj * view;

        scene.SetSpatialPartitionKind(Spark::ScenePartitionKind::BoundingVolumeHierarchy);
        scene.ApplySpatialPolicyFromFirstMatchingObject();

        Spark::SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = camera.position;
        params.lightDirectionWorld = Spark::Vector3{0.35F, 0.88F, 0.32F}.Normalized();
        params.lightColor = {1.0F, 0.95F, 0.88F};
        params.lightIntensity = 0.95F;
        params.ambientColor = {0.10F, 0.11F, 0.14F};
        params.ssaoEnabled = true;

        params.draws.Clear();
        params.sceneTextures.Clear();
        params.pointLights.Clear();
        params.decals.Clear();
        params.sprites.Clear();
        params.screenRects.Clear();
        params.screenTexts.Clear();
        params.screenOverlayRects.Clear();
        params.screenOverlayTexts.Clear();
        params.screenLateRects.Clear();
        params.screenLateTexts.Clear();
        params.uiFont = world.GetUiFont();
        params.uiBoldFont = world.GetUiBoldFont();
        params.draws.Reserve(16);

        scene.ForEachPointLight([&params](const Spark::PointLightComponent& pl, const Spark::Matrix4& worldMat) {
            if (params.pointLights.GetSize() >= Spark::SceneRenderParams::MaxPointLights) {
                return;
            }
            Spark::ScenePointLight gpu{};
            gpu.positionWorld = worldMat.TranslationVector();
            gpu.range = pl.GetRange();
            gpu.color = pl.GetColor();
            gpu.intensity = pl.GetIntensity();
            gpu.castsShadow = pl.CastsShadow();
            params.pointLights.PushBack(gpu);
        });

        const auto findOrAddTexture = [&params](const Spark::SharedPtr<Spark::Texture2D>& tex) -> std::int32_t {
            return Spark::SceneSubmitDetail::FindOrAddSceneTexture(params, tex);
        };

        Spark::Array<Spark::SceneDrawItem> drawList;
        drawList.Reserve(32);
        scene.ForEachDrawableInViewFrustum(viewProj, [&](Spark::GameObject* obj, const Spark::MeshComponent& mc,
                                                              const Spark::MaterialComponent* mat,
                                                              const Spark::Matrix4& world) {
            Spark::SceneDrawItem baseItem{};
            baseItem.model = world;
            baseItem.mesh = mc.GetSlot();
            baseItem.albedo = mc.GetAlbedo();
            baseItem.textureLayer = -1;
            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom) {
                baseItem.customMesh = mc.GetMesh();
            }
            const Spark::MultiMaterialComponent* multiMat =
                    obj != nullptr ? obj->GetComponent<Spark::MultiMaterialComponent>() : nullptr;
            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom && mc.GetMesh() && multiMat != nullptr &&
                !mc.GetMesh()->GetSubmeshes().IsEmpty()) {
                Spark::SceneSubmitDetail::PushRigidMeshDraws(
                        drawList, baseItem, *mc.GetMesh(), mat, multiMat, params, findOrAddTexture);
                return;
            }
            Spark::SceneDrawItem item = baseItem;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                if (mat->GetBaseColorTexture()) {
                    const Spark::Vector3& t = mat->GetTint();
                    item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
                    item.textureLayer = findOrAddTexture(mat->GetBaseColorTexture());
                }
            }
            drawList.PushBack(item);
        });

        scene.ForEachSkinnedDrawableInViewFrustum(viewProj, [&](Spark::GameObject* obj,
                                                                     const Spark::SkinnedMeshComponent& smc,
                                                                     const Spark::MaterialComponent* mat,
                                                                     const Spark::AnimatorComponent* anim,
                                                                     const Spark::Matrix4& world) {
            if (!smc.GetMesh() || anim == nullptr || !anim->GetSkeleton()) {
                return;
            }
            const std::uint32_t jc = anim->GetSkeleton()->GetJointCount();
            if (jc == 0) {
                return;
            }
            const Spark::MultiMaterialComponent* multiMat =
                    obj != nullptr ? obj->GetComponent<Spark::MultiMaterialComponent>() : nullptr;
            Spark::SceneDrawItem baseItem{};
            baseItem.model = world;
            baseItem.mesh = Spark::SceneMeshSlot::Custom;
            baseItem.skinnedMesh = smc.GetMesh();
            baseItem.albedo = {0.9F, 0.88F, 0.82F};
            baseItem.textureLayer = -1;
            baseItem.metallic = 0.0F;
            baseItem.roughness = 0.5F;
            baseItem.jointPalette.Resize(jc);
            anim->ComputeJointPalette(baseItem.jointPalette.GetData(), Spark::Skeleton::MaxJoints);
            if (multiMat != nullptr && !smc.GetMesh()->GetSubmeshes().IsEmpty()) {
                Spark::SceneSubmitDetail::PushSkinnedMeshDraws(
                        drawList, baseItem, *smc.GetMesh(), mat, multiMat, params, findOrAddTexture);
                return;
            }
            Spark::SceneDrawItem item = baseItem;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                if (mat->GetBaseColorTexture()) {
                    const Spark::Vector3& t = mat->GetTint();
                    item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
                    item.textureLayer = findOrAddTexture(mat->GetBaseColorTexture());
                }
            }
            drawList.PushBack(item);
        });

        StableSortDrawItems(drawList);
        for (std::size_t di = 0; di < drawList.GetSize(); ++di) {
            params.draws.PushBack(drawList[di]);
        }

        world.ForEachActiveGameObject([&params, &findOrAddTexture](Spark::GameObject* o) {
            if (o == nullptr || params.decals.GetSize() >= Spark::SceneRenderParams::MaxDecals) {
                return;
            }
            const Spark::DecalProjectorComponent* decalComp = o->GetComponent<Spark::DecalProjectorComponent>();
            if (decalComp == nullptr || !decalComp->IsEnabled()) {
                return;
            }
            Spark::SceneDecalDraw draw{};
            draw.projectorWorld = o->GetWorldMatrix();
            const Spark::Vector3 size = decalComp->GetSize();
            draw.halfExtents = {size.x * 0.5F, size.y * 0.5F, size.z * 0.5F};
            draw.opacity = decalComp->GetOpacity();
            if (decalComp->GetTexture()) {
                draw.textureLayer = findOrAddTexture(decalComp->GetTexture());
            }
            params.decals.PushBack(draw);
        });

        scene.ForEachTextOverlay([&params](const Spark::TextOverlayComponent& tc) {
            Spark::ScreenTextDraw d{};
            d.text = tc.GetText();
            d.x = tc.GetScreenX();
            d.y = tc.GetScreenY();
            d.sizePixels = tc.GetFontSizePixels();
            d.color = tc.GetColor();
            d.alpha = tc.GetAlpha();
            d.paintOrder = params.NextUiPaintOrder();
            params.screenTexts.PushBack(Spark::MoveTemp(d));
        });

        context.SetSceneRenderParams(params);
    }
}  // namespace Spark
