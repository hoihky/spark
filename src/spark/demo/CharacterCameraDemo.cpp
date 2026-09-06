#include "spark/demo/CharacterCameraDemo.hpp"
#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/demo/DemoFoundation.hpp"

#include "spark/audio/SoundFileLoader.hpp"
#include "spark/audio/SoundEngine.hpp"

#include "spark/ecs/components/animation/AnimationEventReceiverComponent.hpp"
#include "spark/ecs/components/animation/AnimationMeleeHitComponent.hpp"
#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/gameplay/DamageableComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/camera/SpringArm3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/world/SceneSpatialPolicyComponent.hpp"
#include "spark/scene/material/GltfMaterial.hpp"
#include "spark/physics/CharacterController3D.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

#include <algorithm>
#include <cmath>
#include <print>

namespace Spark {

namespace {

/** Character camera demo ground: scale the built-in 64×64 m GroundPlane slot by this factor. */
constexpr float kCharGroundLayoutScale = 3.0F;
constexpr float kCharGroundHalfExtent = Spark::kSceneGroundHalfExtent * kCharGroundLayoutScale;
constexpr float kCharTreeGroundInset = 8.0F;
constexpr float kCharTreeMaxRadius = kCharGroundHalfExtent - kCharTreeGroundInset;
/** Ground plane mesh vertices sit at Y=0 (see Mesh::CreateGroundPlane). */
constexpr float kCharGroundPlaneY = 0.0F;
/**
 * Billboard tree glTF roots sit at the trunk base (local Y=0). Do not use mesh AABB min Y — crossed
 * card geometry extends below the visible trunk and causes floating if aligned to boundsMin.
 */
constexpr float kTreeGroundAnchorLocalY = 0.0F;

struct CharTreePlacement {
    float x = 0.0F;
    float z = 0.0F;
    float yawRadians = 0.0F;
    float targetHeightM = 4.0F;
    bool useLargeTree = true;
};

/** Normalized XZ offset in [-1, 1] before scaling to @ref kCharTreeMaxRadius. */
struct CharTreeLayoutSpec {
    float normX = 0.0F;
    float normZ = 0.0F;
    float yawRadians = 0.0F;
    float targetHeightM = 4.0F;
    bool useLargeTree = true;
};

[[nodiscard]] CharTreePlacement MakeTreePlacement(const CharTreeLayoutSpec& spec) noexcept {
    CharTreePlacement out{};
    float x = spec.normX * kCharTreeMaxRadius;
    float z = spec.normZ * kCharTreeMaxRadius;
    const float radiusSq = x * x + z * z;
    const float maxRadiusSq = kCharTreeMaxRadius * kCharTreeMaxRadius;
    if (radiusSq > maxRadiusSq && radiusSq > 1.0e-8F) {
        const float shrink = kCharTreeMaxRadius / std::sqrt(radiusSq);
        x *= shrink;
        z *= shrink;
    }
    out.x = x;
    out.z = z;
    out.yawRadians = spec.yawRadians;
    out.targetHeightM = spec.targetHeightM;
    out.useLargeTree = spec.useLargeTree;
    return out;
}

[[nodiscard]] Spark::GameObject* SpawnCharacterCameraTree(
        Spark::GameWorld& w,
        Spark::Array<Spark::GameObject*>& roots,
        const Spark::GltfAsset& asset,
        const CharTreePlacement& placement,
        const char* name) {
    if (!asset.mesh) {
        return nullptr;
    }

    Spark::Vector3 boundsMin{};
    Spark::Vector3 boundsMax{};
    float uniformScale = 1.0F;
    if (asset.mesh->TryComputeAxisAlignedBounds(boundsMin, boundsMax)) {
        const float height = std::max(1.0e-4F, boundsMax.y - boundsMin.y);
        uniformScale = placement.targetHeightM / height;
    }

    Spark::GameObject* tree = w.CreateGameObject();
    tree->GetName() = Spark::Utf8String(name);
    Spark::TransformComponent* tr = tree->AddComponent<Spark::TransformComponent>();
    tr->SetUniformScale(uniformScale);
    tr->SetTranslation({
            placement.x,
            kCharGroundPlaneY - kTreeGroundAnchorLocalY * uniformScale,
            placement.z});
    tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitY, placement.yawRadians));
    const Spark::Vector3 meshAlbedo =
            (asset.material.HasAnyTexture() || asset.baseColorTexture)
                    ? Spark::Vector3::One
                    : Spark::Vector3{0.42F, 0.68F, 0.34F};
    tree->AddComponent<Spark::MeshComponent>(asset.mesh, Spark::SceneMeshSlot::Custom, meshAlbedo);
    if (asset.material.HasAnyTexture() || asset.baseColorTexture) {
        if (Spark::MaterialComponent* mat = tree->AddComponent<Spark::MaterialComponent>()) {
            Spark::ApplyGltfMaterialDesc(*mat, asset.material);
        }
    } else if (Spark::MaterialComponent* mat = tree->AddComponent<Spark::MaterialComponent>()) {
        mat->SetTint(meshAlbedo);
        mat->SetMetallic(0.0F);
        mat->SetRoughness(0.72F);
    }

    if (placement.useLargeTree) {
        tree->AddComponent<Spark::CapsuleCollider3DComponent>(
                0.55F,
                6.0F,
                Spark::CapsuleDirection3D::Y,
                Spark::Vector3{0.0F, 3.0F, 0.0F});
    } else {
        tree->AddComponent<Spark::CapsuleCollider3DComponent>(
                0.4F,
                2.6F,
                Spark::CapsuleDirection3D::Y,
                Spark::Vector3{0.0F, 1.3F, 0.0F});
    }

    roots.PushBack(tree);
    return tree;
}

}  // namespace

void CharacterCameraDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        characterRoot = nullptr;
        characterRootTr = nullptr;
        characterVisual = nullptr;
        characterVisualTr = nullptr;
        characterController = nullptr;
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

        const float groundUvRepeat = DemoAssets::ProceduralTextureSpanWorldUnits(kCharGroundHalfExtent);
        groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("CharGroundMesh"));
        *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent, groundUvRepeat);
        w.RegisterMesh(groundAsset, kGround);

        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("CharCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        w.RegisterMesh(unitCubeAsset, kCube);

        skyBoxMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("CharSkySphere"));
        *skyBoxMesh = Spark::Mesh::CreateSkySphere(1.0F, 16, 32);
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
        Spark::TransformComponent* groundTr = ground->AddComponent<Spark::TransformComponent>();
        groundTr->SetUniformScale(kCharGroundLayoutScale);
        ground->AddComponent<Spark::MeshComponent>(
                groundAsset, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3::One);
        if (Spark::MaterialComponent* gm = ground->AddComponent<Spark::MaterialComponent>(
                    groundDiffTex, Spark::Vector3::One)) {
            gm->SetMetallic(0.0F);
            gm->SetRoughness(0.82F);
        }
        ground->AddComponent<Spark::SceneSpatialPolicyComponent>(
                Spark::ScenePartitionKind::BoundingVolumeHierarchy);
        roots.PushBack(ground);

        SpawnForestTrees(w);

        characterRoot = w.CreateGameObject();
        characterRoot->GetName() = Spark::Utf8String("Player");
        characterRootTr = characterRoot->AddComponent<Spark::TransformComponent>();
        characterRootTr->SetTranslation(rig.characterPosition);
        characterRootTr->SetRotation(
                Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitY, rig.characterVisualYaw));
        characterController = characterRoot->AddComponent<Spark::CharacterController3DComponent>(
                0.36F, Spark::Vector3{0.0F, 0.88F, 0.0F});
        characterController->SetGravityScale(0.0F);
        characterController->SetStepOffset(0.2F);
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
            SpawnMeleeTrainingDummies(w);
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

        helpHud.Mount(w, "Character camera");
        helpHud.SetControlHints("WASD walk · Shift sprint · F melee (anim events) · H hurt · M model · V FP · F1");

        Spark::GameObject* springArmRig = w.CreateGameObject();
        springArmRig->GetName() = Spark::Utf8String("CharSpringArmRig");
        springArmRig->SetParent(characterRoot);
        Spark::TransformComponent* armTr = springArmRig->AddComponent<Spark::TransformComponent>();
        armTr->SetTranslation({0.0F, 0.0F, 0.0F});
        Spark::SpringArm3DComponent* springArm = springArmRig->AddComponent<Spark::SpringArm3DComponent>();
        springArm->SetPivotTarget(characterRoot);
        springArm->SetSocketOffset({0.0F, 1.55F, 0.0F});
        springArm->SetArmLength(4.2F);
        springArm->SetPitchRadians(-0.22F);
        roots.PushBack(springArmRig);

        if (audioEngine != nullptr) {
            audioEngine->ClearBackgroundMusic();
            audioEngine = nullptr;
        }
        audioEngine = context.TryGetSoundEngine();
        if (audioEngine != nullptr && audioEngine->IsRunning()) {
            if (Spark::SharedPtr<Spark::SoundClip> bgm =
                        Spark::TryLoadSoundClipFromBundledAsset("assets/audio/time_for_adventure.mp3")) {
                audioEngine->SetBackgroundMusic(bgm, 0.28F, true);
            }
        }

        context.GetInput().SetCursorCaptured(true);
    }

void CharacterCameraDemo::Unload(Spark::GameWorld& w)
{
        helpHud.Unmount(w);
        if (audioEngine != nullptr) {
            audioEngine->ClearBackgroundMusic();
            audioEngine = nullptr;
        }
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
        characterController = nullptr;
        playerAnimator = nullptr;
        charAnimFsm = nullptr;
        animEventReceiver = nullptr;
        meleeHit = nullptr;
        meleeTargets.Clear();
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
        const Spark::Vector3 posBeforeWalk = rig.characterPosition;
        rig.ProcessWalk(in, timing.deltaTimeSeconds);
        Spark::Vector3 wishVel = Spark::Vector3::Zero;
        if (timing.deltaTimeSeconds > 1.0e-6F) {
            wishVel = (rig.characterPosition - posBeforeWalk) * (1.0F / timing.deltaTimeSeconds);
            wishVel.y = 0.0F;
        }
        rig.characterPosition = posBeforeWalk;

        if (characterController != nullptr && characterRoot != nullptr && characterRootTr != nullptr) {
            characterRootTr->SetTranslation(rig.characterPosition);
            characterController->SetMoveInput(wishVel);
            Spark::CharacterController3DSettings ccSettings{};
            ccSettings.gravityY = 0.0F;
            ccSettings.maxFallSpeed = 0.0F;
            ccSettings.broadPhaseCellSize = 4.0F;
            Spark::SimulateCharacterControllers3D(characterRoot->GetWorld(), timing, ccSettings);
            const Spark::Vector3 resolved = characterRootTr->GetLocalTransform().translation;
            rig.characterPosition.x = resolved.x;
            rig.characterPosition.y = rig.groundY;
            rig.characterPosition.z = resolved.z;
            characterRootTr->SetTranslation(rig.characterPosition);
        } else if (timing.deltaTimeSeconds > 1.0e-6F) {
            rig.characterPosition.x = posBeforeWalk.x + wishVel.x * timing.deltaTimeSeconds;
            rig.characterPosition.z = posBeforeWalk.z + wishVel.z * timing.deltaTimeSeconds;
            rig.characterPosition.y = rig.groundY;
        }

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
            if (in.IsKeyPressedThisFrame(GLFW_KEY_H)) {
                charAnimFsm->RequestHurt();
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
        {
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
                    if (charAnimFsm->IsDead()) {
                        driveLabel = "dead";
                    } else if (charAnimFsm->IsManualClipActive()) {
                        driveLabel = "manual";
                    } else if (moving) {
                        driveLabel = sprint ? "run" : "walk";
                    }
                }
                Spark::Utf8String blendHud;
                if (playerAnimator->IsLocomotionBlending()) {
                    blendHud = Spark::Utf8String(
                            std::format(
                                    " blend {:.0f}%",
                                    playerAnimator->GetLocomotionBlend01() * 100.0F)
                                    .c_str());
                }
                Spark::Utf8String meleeHud;
                if (meleeHit != nullptr) {
                    meleeHud = Spark::Utf8String(
                            std::format(
                                    " melee:{} hits:{}",
                                    meleeHit->IsHitWindowActive() ? "ON" : "off",
                                    meleeHit->GetTotalHits())
                                    .c_str());
                }
                animHud = Spark::Utf8String(
                        std::format(
                                " — {}/{} clip {} ({}) {} {} [{}]{}{}",
                                playerAnimator->GetClipIndex() + 1,
                                playerAnimator->GetClipCount(),
                                playerAnimator->GetClipIndex(),
                                clipName.CStr(),
                                loopLabel,
                                playerAnimator->IsClipFinished() ? "[finished]" : "",
                                driveLabel,
                                blendHud.CStr(),
                                meleeHud.CStr())
                                .c_str());
            }
            helpHud.SetDetail(
                    std::format("{} — {}{}", modeLabel, characterAvatarHudName.CStr(), animHud.CStr()).c_str());
        }
        helpHud.Update(timing, context);
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
        params.punctualShadowsEnabled = false;

        params.draws.Clear();
        params.sceneTextures.Clear();
        params.sceneHdrTextures.Clear();
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
        params.draws.Reserve(160);

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

        auto findOrAddTexture =
                [&params](const Spark::SharedPtr<Spark::Texture2D>& tex) -> std::int32_t {
            return Spark::SceneSubmitDetail::FindOrAddSceneTexture(params, tex, nullptr, nullptr);
        };

        Spark::Array<Spark::SceneDrawItem> drawList;
        drawList.Reserve(128);

        scene.ForEachSky([&](Spark::GameObject&, const Spark::SkyComponent& sk, const Spark::MeshComponent& mc,
                                 const Spark::MaterialComponent* mat, const Spark::Matrix4& world) {
            Spark::SceneDrawItem item{};
            Spark::SceneSubmitDetail::PopulateSkyDrawItem(item, sk, mc, mat, world, params);
            drawList.PushBack(item);
        });

        scene.ForEachDrawableInViewFrustum(viewProj, [&](Spark::GameObject* obj, const Spark::MeshComponent& mc,
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
            if (mc.GetSlot() == Spark::SceneMeshSlot::GroundPlane) {
                item.doubleSided = true;
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

        helpHud.PatchSceneRenderParams(params, world);
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
        skyTransform->SetUniformScale(98.0F * kCharGroundLayoutScale);
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
        light->AddComponent<Spark::PointLightComponent>(color, intensity, range)->SetCastsShadow(false);
        roots.PushBack(light);
    }

const Spark::SkinnedGltfAsset& CharacterCameraDemo::CachedAvatarAsset(const CharAvatarModel model) const noexcept {
    return model == CharAvatarModel::Fox ? foxAssetCached : cesiumAssetCached;
}

bool CharacterCameraDemo::IsAvatarAssetReady(const CharAvatarModel model) const noexcept {
    return model == CharAvatarModel::Fox ? foxAssetReady : cesiumAssetReady;
}

void CharacterCameraDemo::SpawnForestTrees(Spark::GameWorld& w) {
    Spark::Utf8String treeLargePath(SPARK_ASSETS_DIR);
    treeLargePath.AppendUtf8("/models/Tree_1_C_Color1.gltf");
    Spark::Utf8String treeSmallPath(SPARK_ASSETS_DIR);
    treeSmallPath.AppendUtf8("/models/Tree_3_A_Color1.gltf");

    Spark::GltfAsset treeLarge{};
    Spark::GltfAsset treeSmall{};
    (void)w.AwaitGltf(treeLargePath.CStr(), treeLarge);
    (void)w.AwaitGltf(treeSmallPath.CStr(), treeSmall);
    if (!treeLarge.mesh && !treeSmall.mesh) {
        std::println(
                std::cerr,
                "Spark: CharacterCamera — tree glTF not loaded from {} / {}",
                treeLargePath.CStr(),
                treeSmallPath.CStr());
        return;
    }

    if (treeLarge.baseColorTexture) {
        w.RegisterTexture(treeLarge.baseColorTexture, "spark/char/tree_large_basecolor");
    }
    if (treeSmall.baseColorTexture) {
        w.RegisterTexture(treeSmall.baseColorTexture, "spark/char/tree_small_basecolor");
    } else if (treeLarge.baseColorTexture) {
        w.RegisterTexture(treeLarge.baseColorTexture, "spark/char/tree_small_basecolor");
    }

    static constexpr CharTreeLayoutSpec kTreeLayouts[] = {
            {0.72F, 0.55F, 0.35F, 6.8F, true},
            {-0.68F, 0.58F, 1.85F, 6.2F, true},
            {0.64F, -0.22F, 2.65F, 7.0F, true},
            {-0.62F, -0.66F, 4.1F, 6.5F, true},
            {0.22F, 0.74F, 5.2F, 6.9F, true},
            {-0.28F, -0.72F, 0.9F, 6.4F, true},
            {0.58F, -0.62F, 3.4F, 6.6F, true},
            {-0.70F, 0.18F, 2.1F, 7.1F, true},
            {0.52F, -0.70F, 1.4F, 6.7F, true},
            {-0.16F, 0.76F, 3.8F, 6.3F, true},
            {0.48F, 0.38F, 0.35F, 6.8F, true},
            {-0.52F, 0.32F, 1.85F, 6.2F, true},
            {0.55F, -0.18F, 2.65F, 7.0F, true},
            {-0.44F, -0.46F, 4.1F, 6.5F, true},
            {0.18F, 0.58F, 5.2F, 6.9F, true},
            {-0.24F, -0.54F, 0.9F, 6.4F, true},
            {0.42F, -0.48F, 3.4F, 6.6F, true},
            {-0.58F, 0.12F, 2.1F, 7.1F, true},
            {0.34F, 0.16F, 1.2F, 3.6F, false},
            {-0.36F, 0.22F, 0.6F, 3.4F, false},
            {0.26F, -0.30F, 4.8F, 3.5F, false},
            {-0.22F, -0.26F, 3.1F, 3.3F, false},
            {0.38F, -0.10F, 2.4F, 3.7F, false},
            {-0.40F, -0.06F, 5.6F, 3.5F, false},
            {0.14F, 0.42F, 1.7F, 3.6F, false},
            {-0.30F, 0.48F, 0.2F, 3.4F, false},
            {0.50F, 0.06F, 3.9F, 3.8F, false},
            {-0.12F, 0.52F, 2.8F, 3.5F, false},
            {0.32F, -0.38F, 4.4F, 3.6F, false},
            {-0.46F, -0.32F, 1.5F, 3.7F, false},
            {0.44F, 0.62F, 2.2F, 3.5F, false},
            {-0.56F, 0.42F, 4.6F, 3.6F, false},
            {0.66F, 0.30F, 1.1F, 3.7F, false},
            {-0.34F, -0.60F, 3.3F, 3.4F, false},
            {0.10F, -0.50F, 5.0F, 3.8F, false},
            {-0.48F, -0.16F, 0.8F, 3.5F, false},
    };

    std::size_t spawned = 0;
    for (const CharTreeLayoutSpec& layout : kTreeLayouts) {
        const CharTreePlacement placement = MakeTreePlacement(layout);
        Spark::GltfAsset asset = placement.useLargeTree ? treeLarge : treeSmall;
        if (!asset.baseColorTexture && treeLarge.baseColorTexture) {
            asset.baseColorTexture = treeLarge.baseColorTexture;
        }
        const char* name = placement.useLargeTree ? "CharTreeLarge" : "CharTreeSmall";
        if (SpawnCharacterCameraTree(w, roots, asset, placement, name) != nullptr) {
            ++spawned;
        }
    }

    std::println(
            std::cerr,
            "Spark: CharacterCamera — spawned {} trees (large mesh={}, small mesh={}, forest texture={})",
            spawned,
            treeLarge.mesh ? "ok" : "missing",
            treeSmall.mesh ? "ok" : "missing",
            (treeLarge.baseColorTexture || treeSmall.baseColorTexture) ? "ok" : "missing");
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
    humanModelYawOffset = isFox ? Spark::HalfPi : asset.bindFacingYawOffset + Spark::Pi;
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
        if (asset.material.HasAnyTexture() || asset.baseColorTexture) {
            characterMaterial = characterVisual->AddComponent<Spark::MaterialComponent>();
            Spark::ApplyGltfMaterialDesc(*characterMaterial, asset.material);
        } else {
            characterMaterial = characterVisual->AddComponent<Spark::MaterialComponent>();
        }
    } else {
        characterSkinnedMesh->SetMesh(asset.mesh);
        if (playerAnimator != nullptr) {
            playerAnimator->RetargetSkeleton(asset.skeleton, asset.walkClipIndex, 1.0F);
        }
        if (characterMaterial != nullptr) {
            Spark::ApplyGltfMaterialDesc(*characterMaterial, asset.material);
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
        if (asset.skeleton) {
            const char* attackHint = isFox ? "run" : "walk";
            if (const std::int32_t attackIdx = asset.skeleton->FindClipIndexIfNameContains(attackHint); attackIdx >= 0) {
                charAnimFsm->SetAttackClip(static_cast<std::uint32_t>(attackIdx));
            }
        }
    }

    SetupMeleeCombatComponents(asset, isFox);
}

void CharacterCameraDemo::SpawnMeleeTrainingDummies(Spark::GameWorld& w) {
    if (meleeTargets.GetSize() > 0 || !unitCubeAsset) {
        return;
    }
    const Spark::Vector3 positions[] = {
            Spark::Vector3{2.2F, 0.0F, -2.0F},
            Spark::Vector3{-2.2F, 0.0F, -3.5F},
    };
    for (std::size_t i = 0; i < 2; ++i) {
        Spark::GameObject* dummy = w.CreateGameObject();
        dummy->GetName() = Spark::Utf8String(i == 0 ? "MeleeDummyA" : "MeleeDummyB");
        Spark::TransformComponent* tr = dummy->AddComponent<Spark::TransformComponent>();
        tr->SetTranslation(positions[i]);
        tr->SetUniformScale(1.0F);
        dummy->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.85F, 1.75F, 0.85F});
        if (Spark::MaterialComponent* mat = dummy->AddComponent<Spark::MaterialComponent>()) {
            mat->SetTint(Spark::Vector3(0.82F, 0.28F, 0.24F));
            mat->SetRoughness(0.62F);
        }
        Spark::HealthComponent* hp = dummy->AddComponent<Spark::HealthComponent>(100.0F);
        dummy->AddComponent<Spark::DamageableComponent>();
        hp->SetOnDeath([](Spark::GameObject& self, Spark::GameObject* /*instigator*/) {
            if (Spark::TransformComponent* t = self.GetComponent<Spark::TransformComponent>()) {
                t->SetUniformScale(0.65F);
            }
            if (Spark::MaterialComponent* m = self.GetComponent<Spark::MaterialComponent>()) {
                m->SetTint(Spark::Vector3(0.25F, 0.25F, 0.28F));
            }
        });
        roots.PushBack(dummy);
        meleeTargets.PushBack(dummy);
    }
    if (meleeHit != nullptr) {
        meleeHit->ClearTargets();
        for (std::size_t i = 0; i < meleeTargets.GetSize(); ++i) {
            meleeHit->AddTarget(meleeTargets[i]);
        }
    }
}

void CharacterCameraDemo::SetupMeleeCombatComponents(const Spark::SkinnedGltfAsset& asset, const bool isFox) {
    if (characterVisual == nullptr || !asset.skeleton) {
        return;
    }
    if (animEventReceiver == nullptr) {
        animEventReceiver = characterVisual->AddComponent<Spark::AnimationEventReceiverComponent>();
    }
    animEventReceiver->ImportFromSkeleton(*asset.skeleton);

    if (meleeHit == nullptr) {
        meleeHit = characterVisual->AddComponent<Spark::AnimationMeleeHitComponent>();
        meleeHit->SetFacingObject(characterRoot);
        meleeHit->SetOriginLocalOffset({0.0F, 1.1F, 0.15F});
        meleeHit->SetTraceDistance(isFox ? 2.0F : 2.4F);
        meleeHit->SetDamagePerHit(34.0F);
    }
    meleeHit->ClearTargets();
    for (std::size_t i = 0; i < meleeTargets.GetSize(); ++i) {
        meleeHit->AddTarget(meleeTargets[i]);
    }
}

}  // namespace Spark
