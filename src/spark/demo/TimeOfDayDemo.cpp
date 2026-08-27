#include "spark/demo/TimeOfDayDemo.hpp"

#include "spark/config.hpp"
#include "spark/ecs/components/world/TimeOfDayDriverComponent.hpp"
#include "spark/ecs/components/rendering/FogVolumeComponent.hpp"
#include "spark/ecs/components/rendering/PostProcessVolumeComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>

namespace Spark {

namespace {

[[nodiscard]] const char* TimeOfDayPhaseLabel(const float t) noexcept {
    if (t < 0.08F || t > 0.92F) {
        return "Night";
    }
    if (t < 0.22F) {
        return "Dawn / sunrise";
    }
    if (t < 0.42F) {
        return "Morning";
    }
    if (t < 0.58F) {
        return "Midday (sunshine)";
    }
    if (t < 0.72F) {
        return "Afternoon";
    }
    if (t < 0.88F) {
        return "Sunset";
    }
    return "Dusk";
}

[[nodiscard]] float Wrap01(float x) noexcept {
    x = std::fmod(x, 1.0F);
    if (x < 0.0F) {
        x += 1.0F;
    }
    return x;
}

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

}  // namespace

namespace {

void VisitObjectTree(Spark::GameObject* object, const std::function<void(Spark::GameObject&)>& visitor) {
    if (object == nullptr) {
        return;
    }
    visitor(*object);
    const Spark::Array<Spark::GameObject*>& children = object->GetChildren();
    for (std::size_t i = 0; i < children.GetSize(); ++i) {
        VisitObjectTree(children[i], visitor);
    }
}

void ExpandBoundsPoint(const Spark::Vector3& point, Spark::Vector3& bmin, Spark::Vector3& bmax, bool& any) noexcept {
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
        const Spark::Matrix4& worldMatrix,
        const Spark::Vector3& localMin,
        const Spark::Vector3& localMax,
        Spark::Vector3& bmin,
        Spark::Vector3& bmax,
        bool& any) noexcept {
    const Spark::Vector3 corners[8] = {
            {localMin.x, localMin.y, localMin.z},
            {localMax.x, localMin.y, localMin.z},
            {localMin.x, localMax.y, localMin.z},
            {localMax.x, localMax.y, localMin.z},
            {localMin.x, localMin.y, localMax.z},
            {localMax.x, localMin.y, localMax.z},
            {localMin.x, localMax.y, localMax.z},
            {localMax.x, localMax.y, localMax.z},
    };
    for (const Spark::Vector3& corner : corners) {
        ExpandBoundsPoint(worldMatrix.TransformPoint(corner), bmin, bmax, any);
    }
}

[[nodiscard]] bool TryComputeSubtreeWorldBounds(Spark::GameObject& root, Spark::Vector3& bmin, Spark::Vector3& bmax) {
    bool any = false;
    VisitObjectTree(&root, [&](Spark::GameObject& object) {
        const Spark::Matrix4 worldMatrix = object.GetWorldMatrix();
        if (const Spark::MeshComponent* meshComp = object.GetComponent<Spark::MeshComponent>()) {
            if (const Spark::SharedPtr<Spark::Mesh>& mesh = meshComp->GetMesh()) {
                Spark::Vector3 localMin{};
                Spark::Vector3 localMax{};
                if (mesh->TryComputeAxisAlignedBounds(localMin, localMax)) {
                    ExpandBoundsWithLocalBox(worldMatrix, localMin, localMax, bmin, bmax, any);
                }
            }
        }
    });
    return any;
}

void FitPivotToGround(Spark::GameObject& pivot, const float targetExtentM) {
    Spark::TransformComponent* tr = pivot.GetComponent<Spark::TransformComponent>();
    if (tr == nullptr) {
        return;
    }
    tr->SetUniformScale(1.0F);
    tr->SetTranslation(Spark::Vector3::Zero);

    Spark::Vector3 bmin{};
    Spark::Vector3 bmax{};
    if (!TryComputeSubtreeWorldBounds(pivot, bmin, bmax)) {
        return;
    }
    const float maxExt = std::max({bmax.x - bmin.x, bmax.y - bmin.y, bmax.z - bmin.z});
    float uniformScale = 1.0F;
    if (maxExt > 1.0e-4F) {
        uniformScale = targetExtentM / maxExt;
    }
    constexpr float kGroundClearance = 0.02F;
    tr->SetUniformScale(uniformScale);
    tr->SetTranslation({0.0F, -bmin.y * uniformScale + kGroundClearance, 0.0F});
}

}  // namespace

bool TimeOfDayDemo::TryPlaceCar(
        Spark::GameWorld& w,
        const char* relativePath,
        const Spark::Vector3& pos,
        const float yawRadians,
        const float targetMaxExtentM) {
    char pathBuf[768]{};
    if (!TryResolveModelPath(relativePath, pathBuf, sizeof(pathBuf))) {
        return false;
    }

    Spark::GameObject* go = w.CreateGameObject();
    go->GetName() = Spark::Utf8String("Lantern");

    Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
    tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitY, yawRadians));
    tr->SetTranslation(pos);

    if (!Spark::GltfAssetBinder::BindFromPath(*go, pathBuf, Spark::SceneMeshSlot::Custom, Spark::Vector3::One)) {
        w.DestroyGameObject(go);
        return false;
    }

    FitPivotToGround(*go, targetMaxExtentM);
    if (tr != nullptr) {
        const Spark::Vector3 grounded = tr->GetLocalTransform().translation;
        tr->SetTranslation({pos.x + grounded.x, grounded.y, pos.z + grounded.z});
    }

    roots.PushBack(go);
    return true;
}

void TimeOfDayDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context) {
    roots.Clear();
    carLoaded = false;
    cycleClockSeconds = 0.32F;
    cycleDurationSeconds = 90.0F;
    timeSpeed = 1.0F;
    animateTime = true;

    skyBoxMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("TodSkySphere"));
    *skyBoxMesh = Spark::Mesh::CreateSkySphere(1.0F, 20, 40);
    groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("TodGround"));
    *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent);

    skyEquirectTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("TodSkyEquirect"));
    skyHasEquirect = false;
    Spark::Texture2D skyDecoded;
    if (Spark::Texture2D::TryLoadFromFile(SPARK_SKY_TEXTURE_PATH, skyDecoded)) {
        *skyEquirectTex = Spark::MoveTemp(skyDecoded);
        skyHasEquirect = true;
        w.RegisterTexture(skyEquirectTex, "spark/demo/tod_sky_equirect");
    } else {
        Spark::Utf8String alt(SPARK_ASSETS_DIR);
        alt.AppendUtf8("/textures/sky/equirect_sky_1k.hdr");
        if (Spark::Texture2D::TryLoadFromFile(alt.CStr(), skyDecoded)) {
            *skyEquirectTex = Spark::MoveTemp(skyDecoded);
            skyHasEquirect = true;
            w.RegisterTexture(skyEquirectTex, "spark/demo/tod_sky_equirect");
        }
    }

    groundObject = w.CreateGameObject();
    groundObject->GetName() = Spark::Utf8String("Ground");
    groundObject->AddComponent<Spark::TransformComponent>();
    groundObject->AddComponent<Spark::MeshComponent>(
            groundAsset, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3{0.38F, 0.42F, 0.34F});
    roots.PushBack(groundObject);

    carLoaded = TryPlaceCar(w, "/models/Lantern.glb", {0.0F, 0.0F, 0.0F}, 0.0F, 2.2F);

    skyObject = w.CreateGameObject();
    skyObject->GetName() = Spark::Utf8String("Sky");
    skyTransform = skyObject->AddComponent<Spark::TransformComponent>();
    skyObject->AddComponent<Spark::MeshComponent>(skyBoxMesh, Spark::Vector3::One);
    sky = skyObject->AddComponent<Spark::SkyComponent>(Spark::SceneSkyMode::Dome);
    skyMat = skyObject->AddComponent<Spark::MaterialComponent>();
    if (skyHasEquirect) {
        skyMat->SetBaseColorTexture(skyEquirectTex);
    }
    roots.PushBack(skyObject);
    UpdateSkyTintForTime(cycleClockSeconds / cycleDurationSeconds);

    Spark::GameObject* driverGo = w.CreateGameObject();
    driverGo->GetName() = Spark::Utf8String("TodDriver");
    timeDriver = driverGo->AddComponent<Spark::TimeOfDayDriverComponent>();
    timeDriver->SetDayLengthSeconds(cycleDurationSeconds);
    timeDriver->SetTimeOfDay(cycleClockSeconds / cycleDurationSeconds);
    timeDriver->SetLooping(false);
    roots.PushBack(driverGo);

    Spark::GameObject* fogGo = w.CreateGameObject();
    fogGo->GetName() = Spark::Utf8String("TodFogVolume");
    fogGo->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 2.0F, 0.0F});
    fogVolume = fogGo->AddComponent<Spark::FogVolumeComponent>();
    fogVolume->SetHalfExtents({14.0F, 5.0F, 14.0F});
    fogVolume->SetFogDensity(0.028F);
    fogVolume->SetFogColor({0.62F, 0.68F, 0.78F});
    roots.PushBack(fogGo);

    Spark::GameObject* postGo = w.CreateGameObject();
    postGo->GetName() = Spark::Utf8String("TodPostVolume");
    postGo->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 2.0F, 0.0F});
    postVolume = postGo->AddComponent<Spark::PostProcessVolumeComponent>();
    postVolume->SetHalfExtents({10.0F, 4.0F, 10.0F});
    postVolume->SetSsaoEnabled(true);
    postVolume->SetExposure(1.12F);
    roots.PushBack(postGo);

    ssaoEnabled = true;
    hudDetailClock = 0.0F;
    hudDetailDirty = true;
    helpHud.Mount(w, "Time of day");
    helpHud.SetControlHints("SPACE pause | +/- speed | R dawn | O SSAO | F1 fly");
    if (!carLoaded) {
        helpHud.SetDetail("Lantern.glb missing — expected at assets/models/Lantern.glb");
    }

    context.GetInput().SetCursorCaptured(true);
    camera.position = {3.2F, 2.6F, 10.5F};
    camera.SnapLookAt({0.0F, 0.55F, 0.0F});
}

void TimeOfDayDemo::Unload(Spark::GameWorld& w) {
    helpHud.Unmount(w);
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    carLoaded = false;
    groundObject = nullptr;
    skyObject = nullptr;
    skyTransform = nullptr;
    sky = nullptr;
    skyMat = nullptr;
    skyEquirectTex.Reset();
    skyHasEquirect = false;
    timeDriver = nullptr;
    fogVolume = nullptr;
    postVolume = nullptr;
}

void TimeOfDayDemo::UpdateSkyTintForTime(const float normalizedTime) {
    if (sky == nullptr) {
        return;
    }
    ResolvedSceneLighting resolved = ResolveSceneLightingFromParams(
            SceneLightingProfile::Outdoor,
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            0.0F,
            true,
            true,
            true,
            true,
            true,
            normalizedTime);
    ApplyTimeOfDayLighting(
            normalizedTime,
            SceneLightingProfile::Outdoor,
            resolved,
            nullptr,
            nullptr,
            nullptr);
    const Spark::Vector3 skyTint{
            std::min(resolved.ambient.skyColor.x * 2.2F, 1.0F),
            std::min(resolved.ambient.skyColor.y * 2.2F, 1.0F),
            std::min(resolved.ambient.skyColor.z * 2.2F, 1.0F)};
    sky->SetTint(skyTint);
}

void TimeOfDayDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context) {
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
    if (in.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
        animateTime = !animateTime;
        hudDetailDirty = true;
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_EQUAL) || in.IsKeyPressedThisFrame(GLFW_KEY_KP_ADD)) {
        timeSpeed = std::min(timeSpeed * 1.35F, 8.0F);
        hudDetailDirty = true;
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_MINUS) || in.IsKeyPressedThisFrame(GLFW_KEY_KP_SUBTRACT)) {
        timeSpeed = std::max(timeSpeed / 1.35F, 0.15F);
        hudDetailDirty = true;
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
        cycleClockSeconds = 0.35F * cycleDurationSeconds;
        hudDetailDirty = true;
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_O)) {
        ssaoEnabled = !ssaoEnabled;
        if (postVolume != nullptr) {
            postVolume->SetSsaoEnabled(ssaoEnabled);
        }
        hudDetailDirty = true;
    }

    if (animateTime) {
        cycleClockSeconds += timing.deltaTimeSeconds * timeSpeed;
        hudDetailClock += timing.deltaTimeSeconds;
    }
    const float timeNorm = Wrap01(cycleClockSeconds / cycleDurationSeconds);
    if (timeDriver != nullptr) {
        timeDriver->SetTimeOfDay(timeNorm);
    }
    UpdateSkyTintForTime(timeNorm);

    if (skyTransform != nullptr) {
        skyTransform->SetTranslation(camera.position);
        skyTransform->SetRotation(Spark::Quaternion::Identity);
        skyTransform->SetUniformScale(92.0F);
    }

    if (hudDetailDirty || (animateTime && hudDetailClock >= 0.35F)) {
        hudDetailClock = 0.0F;
        hudDetailDirty = false;
        RefreshHudDetail(timeNorm);
    }
    helpHud.Update(timing, context);
}

void TimeOfDayDemo::RefreshHudDetail(const float timeNorm) noexcept {
    char detail[200]{};
    if (!carLoaded) {
        std::snprintf(
                detail,
                sizeof(detail),
                "Lantern.glb missing | %s | t=%.2f | %.0fs cycle | %.1fx | SSAO %s",
                TimeOfDayPhaseLabel(timeNorm),
                static_cast<double>(timeNorm),
                static_cast<double>(cycleDurationSeconds),
                static_cast<double>(timeSpeed),
                ssaoEnabled ? "on" : "off");
    } else {
        std::snprintf(
                detail,
                sizeof(detail),
                "%s | t=%.2f | %.0fs cycle | %.1fx | SSAO %s",
                TimeOfDayPhaseLabel(timeNorm),
                static_cast<double>(timeNorm),
                static_cast<double>(cycleDurationSeconds),
                static_cast<double>(timeSpeed),
                ssaoEnabled ? "on" : "off");
    }
    helpHud.SetDetail(detail);
}

void TimeOfDayDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

    const Spark::Matrix4 proj = Spark::Matrix4::PerspectiveVulkan(
            Spark::DegreesToRadians(kTimeOfDayDemoFovYDeg), aspect, 0.12F, 400.0F);
    const Spark::Matrix4 view = camera.ViewMatrix();
    const Spark::Matrix4 viewProj = proj * view;

    const float timeNorm = Wrap01(cycleClockSeconds / cycleDurationSeconds);

    Spark::SceneRenderParams params{};
    params.lightingProfile = SceneLightingProfile::Outdoor;
    params.useTimeOfDay = true;
    params.timeOfDay = timeDriver != nullptr ? timeDriver->GetTimeOfDay() : timeNorm;
    params.directionalShadowsEnabled = true;
    params.shadowDepthSampleFlipV = true;
    params.iblEnabled = true;
    params.iblIntensity = 0.85F;
    params.iblEnvironmentLayer = -1;
    params.iblUseHdrSkyEnvironment = skyHasEquirect;
    params.ssaoEnabled = ssaoEnabled;

    FillStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Spark::Vector3{0.35F, 0.88F, 0.48F}.Normalized(),
            {1.0F, 0.98F, 0.92F},
            0.95F,
            Spark::Vector3{0.42F, 0.52F, 0.38F},
            false,
            {},
            {},
            static_cast<float>(cycleClockSeconds),
            params,
            SceneSpriteSortMode::SortOrderOnly,
            &scene);

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

}  // namespace Spark
