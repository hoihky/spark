#include "spark/demo/CharacterCameraDemo.hpp"
#include "spark/demo/DemoAssetLoad.hpp"

#include "spark/ecs/components/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/MaterialComponent.hpp"
#include "spark/ecs/components/SkinnedMeshComponent.hpp"

namespace Spark {

void CharacterCameraDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        characterRoot = nullptr;
        characterRootTr = nullptr;
        characterVisual = nullptr;
        characterVisualTr = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
        playerAnimator = nullptr;
        useSkinnedAvatar = false;
        skyTransform = nullptr;
        charSkyMesh = nullptr;
        charSkyComp = nullptr;
        charSkyMat = nullptr;
        charSkyHasEquirect = false;
        groundDiffTex.Reset();
        skyBoxMesh.Reset();
        skyEquirectTex.Reset();

        rig = {};
        rig.mode = Spark::CharacterCameraMode::ThirdPerson;
        rig.characterPosition = {0.0F, rig.groundY, 2.5F};
        rig.characterVisualYaw = 0.0F;
        rig.cameraYaw = 0.0F;
        rig.cameraPitch = -0.12F;
        rig.thirdPersonPivotHeight = 1.55F;
        rig.thirdPersonFocusAhead = 0.35F;
        rig.thirdPersonCameraLift = 0.5F;
        rig.characterFacingYawOffset = 0.0F;
        rig.characterRootBindOrientation = Spark::Quaternion::Identity;
        rig.characterLocalForward = Spark::Vector3{0.0F, 0.0F, -1.0F};

        const char* kGround = "spark/char/ground";
        const char* kCube = "spark/char/unit_cube";

        groundDiffTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("CharGroundDiffuse"));
        const bool loadedSoil = DemoAssets::TryLoadCharacterCameraSoilTexture(*groundDiffTex);
        if (!loadedSoil) {
            *groundDiffTex = Spark::Texture2D::CreateSoilPattern(256, 256);
            w.RegisterTexture(groundDiffTex, "spark/char/soil_fallback");
        } else {
            w.RegisterTexture(groundDiffTex, "spark/char/ground_soil");
        }

        const float groundUvRepeat =
                DemoAssets::ProceduralTextureSpanWorldUnits(Spark::kSceneGroundHalfExtent);
        groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("CharGroundMesh"));
        *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent, groundUvRepeat);
        w.RegisterMesh(groundAsset, kGround);

        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("CharCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        w.RegisterMesh(unitCubeAsset, kCube);

        skyBoxMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("CharSkySphere"));
        *skyBoxMesh = Spark::Mesh::CreateSkySphere(1.0F, 20, 40);
        skyEquirectTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("CharSkyEquirect"));
        Spark::Texture2D skyDecoded;
        if (Spark::Texture2D::TryLoadFromFile(SPARK_SKY_TEXTURE_PATH, skyDecoded)) {
            *skyEquirectTex = Spark::MoveTemp(skyDecoded);
            charSkyHasEquirect = true;
            w.RegisterTexture(skyEquirectTex, "spark/char/sky_equirect");
        } else {
            Spark::Utf8String altSky(SPARK_ASSETS_DIR);
            altSky.AppendUtf8("/textures/sky/equirect_sky_1k.hdr");
            if (Spark::Texture2D::TryLoadFromFile(altSky.CStr(), skyDecoded)) {
                *skyEquirectTex = Spark::MoveTemp(skyDecoded);
                charSkyHasEquirect = true;
                w.RegisterTexture(skyEquirectTex, "spark/char/sky_equirect");
            }
        }

        Spark::GameObject* skyObj = w.CreateGameObject();
        skyObj->GetName() = Spark::Utf8String("CharSky");
        skyTransform = skyObj->AddComponent<Spark::TransformComponent>();
        charSkyMesh = skyObj->AddComponent<Spark::MeshComponent>(skyBoxMesh, Spark::Vector3::One);
        charSkyComp = skyObj->AddComponent<Spark::SkyComponent>(Spark::SceneSkyMode::Box);
        charSkyMat = skyObj->AddComponent<Spark::MaterialComponent>();
        roots.PushBack(skyObj);
        ApplyCharacterSkyVisuals();

        Spark::GameObject* ground = w.CreateGameObject();
        ground->GetName() = Spark::Utf8String("CharGround");
        ground->AddComponent<Spark::TransformComponent>();
        ground->AddComponent<Spark::MeshComponent>(
                groundAsset, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3::One);
        if (Spark::MaterialComponent* gm = ground->AddComponent<Spark::MaterialComponent>(
                    groundDiffTex, Spark::Vector3::One)) {
            gm->SetMetallic(0.0F);
            gm->SetRoughness(0.82F);
        }
        roots.PushBack(ground);

        characterRoot = w.CreateGameObject();
        characterRoot->GetName() = Spark::Utf8String("Player");
        characterRootTr = characterRoot->AddComponent<Spark::TransformComponent>();
        characterRootTr->SetTranslation(rig.characterPosition);
        characterRootTr->SetRotation(
                Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitY, rig.characterVisualYaw));
        roots.PushBack(characterRoot);

        characterVisualFootOffsetY = 0.0F;
        humanModelYawOffset = 0.0F;
        humanModelBindFix = Spark::Quaternion::Identity;
        characterAvatarHudName = Spark::Utf8String{};
        characterSkinnedMesh = nullptr;
        characterMaterial = nullptr;
        foxAssetReady = false;
        cesiumAssetReady = false;

        Spark::Utf8String cesiumPath(SPARK_ASSETS_DIR);
        cesiumPath.AppendUtf8("/models/CesiumMan.glb");
        Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
        foxPath.AppendUtf8("/models/Fox.glb");
        cesiumAssetCached = w.LoadSkinnedGltf(cesiumPath.CStr());
        foxAssetCached = w.LoadSkinnedGltf(foxPath.CStr());
        cesiumAssetReady = cesiumAssetCached.mesh && cesiumAssetCached.skeleton;
        foxAssetReady = foxAssetCached.mesh && foxAssetCached.skeleton;
        if (cesiumAssetCached.baseColorTexture) {
            w.RegisterTexture(cesiumAssetCached.baseColorTexture, "spark/char/cesium_basecolor");
        }
        if (foxAssetCached.baseColorTexture) {
            w.RegisterTexture(foxAssetCached.baseColorTexture, "spark/char/fox_basecolor");
        }

        if (foxAssetReady || cesiumAssetReady) {
            const CharAvatarModel initialModel = foxAssetReady ? CharAvatarModel::Fox : CharAvatarModel::CesiumMan;
            ApplyAvatarModel(initialModel);
        } else {
            useSkinnedAvatar = false;
            rig.characterFacingYawOffset = 0.0F;
            rig.characterRootBindOrientation = Spark::Quaternion::Identity;
            characterAvatarHudName = Spark::Utf8String("Primitives (CesiumMan.glb / Fox.glb missing)");
            playerAnimator = nullptr;
            charAnimFsm = nullptr;
            auto setupPart = [&](const char* name, Spark::Vector3 localPos, Spark::Vector3 localScale, Spark::Vector3 albedo) {
                Spark::GameObject* part = w.CreateGameObject();
                part->GetName() = Spark::Utf8String(name);
                part->SetParent(characterRoot);
                Spark::TransformComponent* tr = part->AddComponent<Spark::TransformComponent>();
                tr->SetTranslation(localPos);
                tr->SetScale(localScale);
                part->AddComponent<Spark::MeshComponent>(
                        unitCubeAsset, Spark::SceneMeshSlot::UnitCube, albedo);
                if (Spark::MaterialComponent* m = part->AddComponent<Spark::MaterialComponent>()) {
                    m->SetTint(albedo);
                    m->SetMetallic(0.06F);
                    m->SetRoughness(0.52F);
                }
                roots.PushBack(part);
            };
            setupPart("CharBody", {0.0F, 0.42F, 0.0F}, {0.34F, 0.82F, 0.24F}, {0.32F, 0.52F, 0.9F});
            setupPart("CharHead", {0.0F, 0.98F, 0.0F}, {0.22F, 0.22F, 0.22F}, {0.92F, 0.72F, 0.58F});
        }

        AddPointLight(w, {12.0F, 14.0F, 10.0F}, {0.95F, 0.9F, 0.75F}, 3.2F, 45.0F);
        AddPointLight(w, {-14.0F, 9.0F, -8.0F}, {0.45F, 0.65F, 1.0F}, 2.4F, 38.0F);

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("CharFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(12.0F, 12.0F);
        fpsText->SetFontSizePixels(20.0F);
        fpsText->SetColor({0.9F, 0.95F, 1.0F});
        fpsText->SetText(Spark::Utf8String(
                std::format(
                        "Character — {} · WASD walk · Shift+WASD run · M model · 1/2/3 clips · V FP · F1",
                        characterAvatarHudName.CStr())
                        .c_str()));
        roots.PushBack(fpsHudObject);

        context.GetInput().SetCursorCaptured(true);
    }

void CharacterCameraDemo::Unload(Spark::GameWorld& w)
{
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        characterRoot = nullptr;
        characterRootTr = nullptr;
        characterVisual = nullptr;
        characterVisualTr = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
        playerAnimator = nullptr;
        charAnimFsm = nullptr;
        characterSkinnedMesh = nullptr;
        characterMaterial = nullptr;
        foxAssetReady = false;
        cesiumAssetReady = false;
        foxAssetCached = {};
        cesiumAssetCached = {};
        useSkinnedAvatar = false;
        characterVisualFootOffsetY = 0.0F;
        humanModelYawOffset = 0.0F;
        humanModelBindFix = Spark::Quaternion::Identity;
        characterAvatarHudName = Spark::Utf8String{};
        skyTransform = nullptr;
        charSkyMesh = nullptr;
        charSkyComp = nullptr;
        charSkyMat = nullptr;
        groundAsset.Reset();
        unitCubeAsset.Reset();
        groundDiffTex.Reset();
        skyBoxMesh.Reset();
        skyEquirectTex.Reset();
    }

void CharacterCameraDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        Spark::IInput& in = context.GetInput();
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_V)) {
            rig.ToggleCameraMode();
        }
        if (useSkinnedAvatar && in.IsKeyPressedThisFrame(GLFW_KEY_M)) {
            const CharAvatarModel next = activeAvatarModel == CharAvatarModel::Fox ? CharAvatarModel::CesiumMan
                                                                                   : CharAvatarModel::Fox;
            if (IsAvatarAssetReady(next)) {
                ApplyAvatarModel(next);
            }
        }
        if (in.IsCursorCaptured()) {
            if (timing.frameIndex > 0) {
                rig.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
            }
        }
        rig.ProcessWalk(in, timing.deltaTimeSeconds);
        const bool moving = in.IsKeyDown(GLFW_KEY_W) || in.IsKeyDown(GLFW_KEY_S) || in.IsKeyDown(GLFW_KEY_A)
                || in.IsKeyDown(GLFW_KEY_D);
        const bool sprint = moving
                && (in.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || in.IsKeyDown(GLFW_KEY_RIGHT_SHIFT));
        if (useSkinnedAvatar && charAnimFsm != nullptr) {
            if (moving) {
                charAnimFsm->ClearManualClip();
            }
            charAnimFsm->SetLocomotionInput(moving, sprint);
        }
        if (useSkinnedAvatar && playerAnimator != nullptr && charAnimFsm != nullptr) {
            const std::uint32_t clipCount = playerAnimator->GetClipCount();
            auto playManualClip = [&](const std::uint32_t clip, const Spark::AnimLoopMode loop) {
                if (clip >= clipCount) {
                    return;
                }
                charAnimFsm->SetManualClip(clip, loop);
                playerAnimator->SetLoopMode(loop);
                playerAnimator->SetClipIndexWithCrossfade(clip, 0.2F);
            };
            if (in.IsKeyPressedThisFrame(GLFW_KEY_1)) {
                playManualClip(0, Spark::AnimLoopMode::Loop);
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_2) && clipCount > 1) {
                playManualClip(1, Spark::AnimLoopMode::Loop);
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_3) && clipCount > 2) {
                playManualClip(2, Spark::AnimLoopMode::Loop);
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_4)) {
                const std::uint32_t cur = playerAnimator->GetClipIndex();
                charAnimFsm->SetManualClip(cur, Spark::AnimLoopMode::Once);
                playerAnimator->SetLoopMode(Spark::AnimLoopMode::Once);
                playerAnimator->RestartCurrentClip();
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_LEFT_BRACKET) && clipCount > 0) {
                const std::uint32_t cur = playerAnimator->GetClipIndex();
                const std::uint32_t next = (cur + clipCount - 1) % clipCount;
                playManualClip(next, Spark::AnimLoopMode::Loop);
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_RIGHT_BRACKET) && clipCount > 0) {
                const std::uint32_t cur = playerAnimator->GetClipIndex();
                const std::uint32_t next = (cur + 1) % clipCount;
                playManualClip(next, Spark::AnimLoopMode::Loop);
            }
            if (in.IsKeyPressedThisFrame(GLFW_KEY_F)) {
                charAnimFsm->RequestAttack();
            }
        }
        if (characterRootTr != nullptr) {
            characterRootTr->SetTranslation(rig.characterPosition);
            characterRootTr->SetUniformScale(1.0F);
            const Spark::Quaternion qYaw = Spark::Quaternion::FromAxisAngle(
                    Spark::Vector3::UnitY, rig.characterVisualYaw + humanModelYawOffset);
            characterRootTr->SetRotation(
                    useSkinnedAvatar ? (qYaw * humanModelBindFix).Normalized() : qYaw);
        }
        if (fpsText != nullptr) {
            const float dt = timing.deltaTimeSeconds;
            const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            const char* modeLabel =
                    rig.mode == Spark::CharacterCameraMode::FirstPerson ? "1st person" : "3rd person";
            Spark::Utf8String animHud;
            if (useSkinnedAvatar && playerAnimator != nullptr) {
                const Spark::Utf8String& clipName = playerAnimator->GetClipName(playerAnimator->GetClipIndex());
                const char* loopLabel = "loop";
                switch (playerAnimator->GetLoopMode()) {
                    case Spark::AnimLoopMode::Once:
                        loopLabel = "once";
                        break;
                    case Spark::AnimLoopMode::Hold:
                        loopLabel = "hold";
                        break;
                    default:
                        break;
                }
                const char* driveLabel = "auto";
                if (charAnimFsm != nullptr) {
                    if (charAnimFsm->IsManualClipActive()) {
                        driveLabel = "manual";
                    } else if (moving) {
                        driveLabel = sprint ? "run" : "walk";
                    }
                }
                animHud = Spark::Utf8String(
                        std::format(
                                " — {}/{} clip {} ({}) {} {} [{}]",
                                playerAnimator->GetClipIndex() + 1,
                                playerAnimator->GetClipCount(),
                                playerAnimator->GetClipIndex(),
                                clipName.CStr(),
                                loopLabel,
                                playerAnimator->IsClipFinished() ? "[finished]" : "",
                                driveLabel)
                                .c_str());
            }
            fpsText->SetText(Spark::Utf8String(
                    std::format(
                            "Character — {:.0f} FPS — {} — {}{} — WASD walk · Shift run · M model · V · F1",
                            static_cast<double>(fpsSmoothed),
                            modeLabel,
                            characterAvatarHudName.CStr(),
                            animHud.CStr())
                            .c_str()));
        }
    }

void CharacterCameraDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        UpdateCharacterSkyTransform();
        scene.ApplySpatialPolicyFromFirstMatchingObject();
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

        const Spark::Matrix4 proj =
                Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 400.0F);
        const Spark::Matrix4 view = rig.ViewMatrix();
        const Spark::Matrix4 viewProj = proj * view;

        Spark::SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = rig.CameraWorldPosition();
        params.lightDirectionWorld = Spark::Vector3{0.38F, 0.82F, 0.35F}.Normalized();
        params.lightColor = {1.0F, 0.97F, 0.9F};
        params.lightIntensity = 0.92F;
        params.ambientColor = {0.10F, 0.12F, 0.15F};

        params.draws.Clear();
        params.sceneTextures.Clear();
        params.pointLights.Clear();
        params.sprites.Clear();
        params.screenRects.Clear();
        params.screenTexts.Clear();
        params.screenOverlayRects.Clear();
        params.screenOverlayTexts.Clear();
        params.screenLateRects.Clear();
        params.screenLateTexts.Clear();
        params.uiFont = world.GetUiFont();
        params.uiBoldFont = world.GetUiBoldFont();
        params.draws.Reserve(48);

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

        auto findOrAddTexture = [&params](const Spark::SharedPtr<Spark::Texture2D>& tex) -> std::int32_t {
            if (!tex) {
                return -1;
            }
            for (std::size_t i = 0; i < params.sceneTextures.GetSize(); ++i) {
                if (params.sceneTextures[i].Get() == tex.Get()) {
                    return static_cast<std::int32_t>(i);
                }
            }
            if (params.sceneTextures.GetSize() >= Spark::SceneRenderParams::MaxSceneTextures) {
                return -1;
            }
            params.sceneTextures.PushBack(tex);
            return static_cast<std::int32_t>(params.sceneTextures.GetSize() - 1U);
        };

        Spark::Array<Spark::SceneDrawItem> drawList;
        drawList.Reserve(32);

        scene.ForEachSky([&](Spark::GameObject&, const Spark::SkyComponent& sk, const Spark::MeshComponent& mc,
                                 const Spark::MaterialComponent* mat, const Spark::Matrix4& world) {
            Spark::SceneDrawItem item{};
            item.mesh = Spark::SceneMeshSlot::Custom;
            item.skyMode = sk.GetSkyMode();
            item.model = world;
            item.customMesh = mc.GetMesh();
            item.albedo = sk.GetTint();
            item.textureLayer = -1;
            item.metallic = 0.0F;
            item.roughness = 1.0F;
            if (mat != nullptr && mat->GetBaseColorTexture()) {
                const Spark::Vector3& t = mat->GetTint();
                item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
                item.textureLayer = findOrAddTexture(mat->GetBaseColorTexture());
            }
            drawList.PushBack(item);
        });

        scene.ForEachDrawable([&](Spark::GameObject* obj, const Spark::MeshComponent& mc,
                                     const Spark::MaterialComponent* mat, const Spark::Matrix4& world) {
            if (obj != nullptr && obj->GetComponent<Spark::SkyComponent>() != nullptr) {
                return;
            }
            Spark::SceneDrawItem item{};
            item.model = world;
            item.mesh = mc.GetSlot();
            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom) {
                item.customMesh = mc.GetMesh();
            }
            Spark::Vector3 alb = mc.GetAlbedo();
            item.textureLayer = -1;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                if (mat->GetBaseColorTexture()) {
                    const Spark::Vector3& t = mat->GetTint();
                    alb = {alb.x * t.x, alb.y * t.y, alb.z * t.z};
                    item.textureLayer = findOrAddTexture(mat->GetBaseColorTexture());
                }
            }
            item.albedo = alb;
            drawList.PushBack(item);
        });

        scene.ForEachSkinnedDrawableInViewFrustum(viewProj, [&](Spark::GameObject* obj,
                                                                     const Spark::SkinnedMeshComponent& smc,
                                                                     const Spark::MaterialComponent* mat,
                                                                     const Spark::AnimatorComponent* anim,
                                                                     const Spark::Matrix4& world) {
            if (rig.mode == Spark::CharacterCameraMode::FirstPerson && characterRoot != nullptr && obj != nullptr
                    && (obj == characterRoot || obj->GetParent() == characterRoot)) {
                return;
            }
            if (!smc.GetMesh() || anim == nullptr || !anim->GetSkeleton()) {
                return;
            }
            const std::uint32_t jc = anim->GetSkeleton()->GetJointCount();
            if (jc == 0) {
                return;
            }
            Spark::SceneDrawItem item{};
            item.model = world;
            item.mesh = Spark::SceneMeshSlot::Custom;
            item.skinnedMesh = smc.GetMesh();
            item.albedo = {0.9F, 0.88F, 0.82F};
            item.textureLayer = -1;
            item.metallic = 0.0F;
            item.roughness = 0.5F;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                if (mat->GetBaseColorTexture()) {
                    const Spark::Vector3& t = mat->GetTint();
                    item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
                    item.textureLayer = findOrAddTexture(mat->GetBaseColorTexture());
                }
            }
            item.jointPalette.Resize(jc);
            anim->ComputeJointPalette(item.jointPalette.GetData(), Spark::Skeleton::MaxJoints);
            drawList.PushBack(item);
        });

        StableSortDrawItems(drawList);
        for (std::size_t di = 0; di < drawList.GetSize(); ++di) {
            params.draws.PushBack(drawList[di]);
        }

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

void CharacterCameraDemo::ApplyCharacterSkyVisuals()
{
        if (charSkyComp == nullptr || charSkyMesh == nullptr || charSkyMat == nullptr) {
            return;
        }
        charSkyComp->SetSkyMode(Spark::SceneSkyMode::Box);
        if (charSkyHasEquirect && skyEquirectTex) {
            charSkyComp->SetTint(Spark::Vector3::One);
            charSkyMesh->SetAlbedo(Spark::Vector3::One);
            charSkyMat->SetBaseColorTexture(skyEquirectTex);
        } else {
            charSkyComp->SetTint({0.22F, 0.34F, 0.58F});
            charSkyMesh->SetAlbedo(charSkyComp->GetTint());
            charSkyMat->SetBaseColorTexture(Spark::SharedPtr<Spark::Texture2D>{});
        }
    }

void CharacterCameraDemo::UpdateCharacterSkyTransform()
{
        if (skyTransform == nullptr) {
            return;
        }
        skyTransform->SetTranslation(rig.CameraWorldPosition());
        skyTransform->SetRotation(Spark::Quaternion::Identity);
        skyTransform->SetUniformScale(98.0F);
    }

void CharacterCameraDemo::AddPointLight(
            Spark::GameWorld& w,
            Spark::Vector3 position,
            Spark::Vector3 color,
            float intensity,
            float range)
{
        Spark::GameObject* light = w.CreateGameObject();
        light->GetName() = Spark::Utf8String("CharPointLight");
        Spark::TransformComponent* tr = light->AddComponent<Spark::TransformComponent>();
        tr->SetTranslation(position);
        light->AddComponent<Spark::PointLightComponent>(color, intensity, range)->SetCastsShadow(true);
        roots.PushBack(light);
    }

const Spark::SkinnedGltfAsset& CharacterCameraDemo::CachedAvatarAsset(const CharAvatarModel model) const noexcept {
    return model == CharAvatarModel::Fox ? foxAssetCached : cesiumAssetCached;
}

bool CharacterCameraDemo::IsAvatarAssetReady(const CharAvatarModel model) const noexcept {
    return model == CharAvatarModel::Fox ? foxAssetReady : cesiumAssetReady;
}

void CharacterCameraDemo::ApplyAvatarModel(const CharAvatarModel model) {
    if (!IsAvatarAssetReady(model) || characterRoot == nullptr || characterRootTr == nullptr) {
        return;
    }

    const Spark::SkinnedGltfAsset& asset = CachedAvatarAsset(model);
    const bool isFox = model == CharAvatarModel::Fox;
    activeAvatarModel = model;
    useSkinnedAvatar = true;

    humanModelBindFix = asset.bindUpAlignment.Normalized();
    humanModelYawOffset = isFox ? -Spark::HalfPi : asset.bindFacingYawOffset;
    rig.characterFacingYawOffset = humanModelYawOffset;
    rig.characterRootBindOrientation = humanModelBindFix;

    Spark::Vector3 alignedMin{};
    Spark::Vector3 alignedMax{};
    bool haveBounds = false;
    float minAlignY = 0.0F;
    {
        const Spark::Array<Spark::SkinnedMesh::Vertex>& sv = asset.mesh->GetVertices();
        if (!sv.IsEmpty()) {
            haveBounds = true;
            bool first = true;
            for (std::size_t vi = 0; vi < sv.GetSize(); ++vi) {
                const Spark::Vector3 p = humanModelBindFix.RotateVector(sv[vi].position);
                if (first) {
                    alignedMin = p;
                    alignedMax = p;
                    minAlignY = p.y;
                    first = false;
                } else {
                    alignedMin.x = std::min(alignedMin.x, p.x);
                    alignedMin.y = std::min(alignedMin.y, p.y);
                    alignedMin.z = std::min(alignedMin.z, p.z);
                    alignedMax.x = std::max(alignedMax.x, p.x);
                    alignedMax.y = std::max(alignedMax.y, p.y);
                    alignedMax.z = std::max(alignedMax.z, p.z);
                    minAlignY = std::min(minAlignY, p.y);
                }
            }
        }
    }

    constexpr float kTargetHeightM = 1.75F;
    float sc = 0.04F;
    if (haveBounds) {
        const float h = std::max(1.0e-4F, alignedMax.y - alignedMin.y);
        sc = kTargetHeightM / h;
    }

    Spark::Vector3 visualLocalOffset{};
    if (haveBounds) {
        characterVisualFootOffsetY = -minAlignY * sc;
        visualLocalOffset = {
                -(alignedMin.x + alignedMax.x) * 0.5F * sc,
                characterVisualFootOffsetY,
                -(alignedMin.z + alignedMax.z) * 0.5F * sc};
    } else {
        characterVisualFootOffsetY = 0.0F;
    }

    if (haveBounds) {
        const float H = (alignedMax.y - alignedMin.y) * sc;
        rig.firstPersonEyeHeight = std::clamp(H * 0.88F, 1.4F, 1.95F);
        rig.firstPersonForwardNudge = std::clamp(H * 0.08F, 0.14F, 0.42F);
        rig.thirdPersonPivotHeight = std::clamp(H * 0.88F, 1.32F, 1.9F);
        rig.thirdPersonFocusAhead = std::clamp(H * 0.05F, 0.12F, 0.38F);
        rig.thirdPersonCameraLift = std::clamp(H * 0.12F, 0.35F, 0.68F);
    } else {
        rig.firstPersonEyeHeight = 1.68F;
        rig.firstPersonForwardNudge = 0.28F;
        rig.thirdPersonPivotHeight = 1.55F;
        rig.thirdPersonFocusAhead = 0.32F;
        rig.thirdPersonCameraLift = 0.5F;
    }

    characterAvatarHudName = isFox ? Spark::Utf8String("Khronos Fox")
                                   : Spark::Utf8String("Khronos CesiumMan robot");

    characterRootTr->SetUniformScale(1.0F);
    characterRootTr->SetRotation(
            (Spark::Quaternion::FromAxisAngle(
                     Spark::Vector3::UnitY, rig.characterVisualYaw + humanModelYawOffset)
             * humanModelBindFix)
                    .Normalized());

    if (characterVisual == nullptr) {
        characterVisual = characterRoot->GetWorld().CreateGameObject();
        characterVisual->GetName() = Spark::Utf8String("PlayerVisual");
        characterVisual->SetParent(characterRoot);
        characterVisualTr = characterVisual->AddComponent<Spark::TransformComponent>();
        characterSkinnedMesh = characterVisual->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);
        charAnimFsm = characterVisual->AddComponent<Spark::Character3DAnimFsmComponent>();
        playerAnimator = characterVisual->AddComponent<Spark::AnimatorComponent>(
                asset.skeleton, asset.walkClipIndex, 1.0F);
        if (asset.baseColorTexture) {
            characterMaterial = characterVisual->AddComponent<Spark::MaterialComponent>(
                    asset.baseColorTexture, Spark::Vector3::One);
            characterMaterial->SetMetallic(0.0F);
            characterMaterial->SetRoughness(0.55F);
        } else {
            characterMaterial = characterVisual->AddComponent<Spark::MaterialComponent>();
        }
    } else {
        characterSkinnedMesh->SetMesh(asset.mesh);
        if (playerAnimator != nullptr) {
            playerAnimator->RetargetSkeleton(asset.skeleton, asset.walkClipIndex, 1.0F);
        }
        if (characterMaterial != nullptr) {
            characterMaterial->SetBaseColorTexture(asset.baseColorTexture);
        }
    }

    if (characterVisualTr != nullptr) {
        characterVisualTr->SetUniformScale(sc);
        characterVisualTr->SetTranslation(visualLocalOffset);
        characterVisualTr->SetRotation(Spark::Quaternion::Identity);
    }

    if (charAnimFsm != nullptr) {
        charAnimFsm->ClearManualClip();
        charAnimFsm->ConfigureLocomotionFromSkeleton(*asset.skeleton, asset.walkClipIndex);
        charAnimFsm->SetWalkSpeedThreshold(0.35F);
        charAnimFsm->SetRunSpeedThreshold(2.5F);
    }
}

}  // namespace Spark
