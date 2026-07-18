#include "spark/demo/TimeOfDayDemo.hpp"

#include "spark/config.hpp"
#include "spark/ecs/components/MaterialComponent.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"

#include <algorithm>
#include <cmath>
#include <format>

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

}  // namespace

void TimeOfDayDemo::SpawnPillar(
        Spark::GameWorld& w, const float x, const float z, const float height, const Spark::Vector3& color) {
    Spark::GameObject* pillar = w.CreateGameObject();
    pillar->GetName() = Spark::Utf8String("Pillar");
    Spark::TransformComponent* tr = pillar->AddComponent<Spark::TransformComponent>();
    tr->SetTranslation({x, height * 0.5F, z});
    tr->SetUniformScale(1.0F);
    tr->SetScale({0.55F, height * 0.5F, 0.55F});
    pillar->AddComponent<Spark::MeshComponent>(
            unitCubeAsset, Spark::SceneMeshSlot::UnitCube, color);
    roots.PushBack(pillar);
}

void TimeOfDayDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context) {
    roots.Clear();
    cycleClockSeconds = 0.32F;
    cycleDurationSeconds = 90.0F;
    timeSpeed = 1.0F;
    animateTime = true;

    skyBoxMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("TodSkySphere"));
    *skyBoxMesh = Spark::Mesh::CreateSkySphere(1.0F, 20, 40);
    groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("TodGround"));
    *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent);
    unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("TodCube"));
    *unitCubeAsset = Spark::Mesh::CreateUnitCube();

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

    SpawnPillar(w, -6.0F, -4.0F, 3.5F, {0.72F, 0.68F, 0.62F});
    SpawnPillar(w, 6.0F, -4.0F, 4.2F, {0.68F, 0.7F, 0.74F});
    SpawnPillar(w, -5.0F, 5.5F, 2.8F, {0.7F, 0.66F, 0.6F});
    SpawnPillar(w, 5.5F, 5.0F, 3.8F, {0.66F, 0.68F, 0.7F});
    SpawnPillar(w, 0.0F, -7.0F, 5.0F, {0.74F, 0.72F, 0.68F});

    Spark::GameObject* center = w.CreateGameObject();
    center->GetName() = Spark::Utf8String("CenterCube");
    {
        Spark::TransformComponent* tr = center->AddComponent<Spark::TransformComponent>();
        tr->SetTranslation({0.0F, 0.6F, 0.0F});
        tr->SetUniformScale(1.2F);
    }
    center->AddComponent<Spark::MeshComponent>(
            unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.85F, 0.55F, 0.3F});
    roots.PushBack(center);

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

    hudObject = w.CreateGameObject();
    hudObject->GetName() = Spark::Utf8String("TodHud");
    hudText = hudObject->AddComponent<Spark::TextOverlayComponent>();
    hudText->SetScreenPosition(12.0F, 12.0F);
    hudText->SetFontSizePixels(20.0F);
    hudText->SetColor({0.92F, 0.94F, 0.98F});
    roots.PushBack(hudObject);

    context.GetInput().SetCursorCaptured(true);
    camera.position = {0.0F, 3.5F, 16.0F};
    camera.SnapLookAt({0.0F, 1.5F, 0.0F});
}

void TimeOfDayDemo::Unload(Spark::GameWorld& w) {
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    groundObject = nullptr;
    skyObject = nullptr;
    skyTransform = nullptr;
    sky = nullptr;
    skyMat = nullptr;
    skyEquirectTex.Reset();
    skyHasEquirect = false;
    hudObject = nullptr;
    hudText = nullptr;
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
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_EQUAL) || in.IsKeyPressedThisFrame(GLFW_KEY_KP_ADD)) {
        timeSpeed = std::min(timeSpeed * 1.35F, 8.0F);
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_MINUS) || in.IsKeyPressedThisFrame(GLFW_KEY_KP_SUBTRACT)) {
        timeSpeed = std::max(timeSpeed / 1.35F, 0.15F);
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
        cycleClockSeconds = 0.35F * cycleDurationSeconds;
    }

    if (animateTime) {
        cycleClockSeconds += timing.deltaTimeSeconds * timeSpeed;
    }
    const float timeNorm = Wrap01(cycleClockSeconds / cycleDurationSeconds);
    UpdateSkyTintForTime(timeNorm);

    if (skyTransform != nullptr) {
        skyTransform->SetTranslation(camera.position);
        skyTransform->SetRotation(Spark::Quaternion::Identity);
        skyTransform->SetUniformScale(92.0F);
    }

    if (hudText != nullptr) {
        const float dt = timing.deltaTimeSeconds;
        const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
        if (timing.frameIndex < 2U) {
            fpsSmoothed = instant;
        } else {
            fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
        }
        hudText->SetText(Spark::Utf8String(
                std::format(
                        "Time of day — {} — t={:.2f} — {:.0f}s/cycle — {:.1f}x — {:.0f} FPS — SPACE pause  +/- speed  R dawn",
                        TimeOfDayPhaseLabel(timeNorm),
                        static_cast<double>(timeNorm),
                        static_cast<double>(cycleDurationSeconds),
                        static_cast<double>(timeSpeed),
                        static_cast<double>(fpsSmoothed))
                        .c_str()));
    }
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
    params.viewProjection = viewProj;
    params.cameraPositionWorld = camera.position;
    params.lightingProfile = SceneLightingProfile::Outdoor;
    params.useTimeOfDay = true;
    params.timeOfDay = timeNorm;
    params.directionalShadowsEnabled = true;
    params.shadowDepthSampleFlipV = true;
    params.sceneTimeSeconds = static_cast<float>(cycleClockSeconds);

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
    params.draws.Reserve(24);

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
    drawList.Reserve(16);

    scene.ForEachSky([&](Spark::GameObject&, const Spark::SkyComponent& sk, const Spark::MeshComponent& mc,
                             const Spark::MaterialComponent* mat, const Spark::Matrix4& worldMatrix) {
        Spark::SceneDrawItem item{};
        item.mesh = Spark::SceneMeshSlot::Custom;
        item.skyMode = sk.GetSkyMode();
        item.model = worldMatrix;
        item.customMesh = mc.GetMesh();
        item.albedo = sk.GetTint();
        item.textureLayer = -1;
        item.shadowFlags = 0;
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
                                 const Spark::MaterialComponent* mat, const Spark::Matrix4& worldMatrix) {
        if (obj != nullptr && obj->GetComponent<Spark::SkyComponent>() != nullptr) {
            return;
        }
        Spark::SceneDrawItem item{};
        item.model = worldMatrix;
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
            }
        }
        item.albedo = alb;
        item.shadowFlags = kSceneShadowCastAndReceive;
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

}  // namespace Spark
