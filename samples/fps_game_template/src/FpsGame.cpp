#include "FpsGame.hpp"

#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/scene/SceneSubmit.hpp"
#include "spark/config.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IRenderFrame.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/text/Font.hpp"

#include <GLFW/glfw3.h>

#include <cmath>
#include <format>
#include <string>

namespace Spark {

namespace {

constexpr float kCubeScale = 1.0F;
constexpr float kTargetHitRadius = 0.58F;
constexpr float kRayMaxDistance = 120.0F;
constexpr float kTracerSpeed = 52.0F;
constexpr float kTracerLifetime = 0.65F;
constexpr float kTracerUniformScale = 0.11F;
constexpr float kMuzzleForwardOffset = 0.42F;
constexpr std::size_t kMaxTracerBullets = 96;

[[nodiscard]] bool RaySphereNearestT(
        const Vector3& rayOrigin, const Vector3& rayDirUnit, const Vector3& center, float radius, float& outT) {
    const Vector3 oc = rayOrigin - center;
    const float b = Vector3::Dot(rayDirUnit, oc);
    const float c = Vector3::Dot(oc, oc) - radius * radius;
    const float discriminant = b * b - c;
    if (discriminant < 0.0F) {
        return false;
    }
    const float s = std::sqrt(discriminant);
    float t = -b - s;
    if (t < 1.0e-3F) {
        t = -b + s;
    }
    if (t < 1.0e-3F || t > kRayMaxDistance) {
        return false;
    }
    outT = t;
    return true;
}

}  // namespace

void FpsGame::MountUiFontIfNeeded(GameWorld& world) {
    if (world.GetUiFont()) {
        return;
    }
    auto uiFont = MakeShared<Font>();
    constexpr float kUiFontEmPx = 42.0F;
    bool fontOk = uiFont->TryLoadTrueTypeFromFile(SPARK_UI_FONT_PATH, kUiFontEmPx);
    if (!fontOk) {
        Utf8String buildTree(SPARK_BUILD_ASSETS_DIR);
        buildTree.AppendUtf8("/fonts/Roboto-Regular.ttf");
        fontOk = uiFont->TryLoadTrueTypeFromFile(buildTree.CStr(), kUiFontEmPx);
    }
    if (!fontOk) {
        Utf8String srcTree(SPARK_ASSETS_DIR);
        srcTree.AppendUtf8("/fonts/Roboto-Regular.ttf");
        fontOk = uiFont->TryLoadTrueTypeFromFile(srcTree.CStr(), kUiFontEmPx);
    }
    if (!fontOk) {
        fontOk = uiFont->TryLoadTrueTypeFromFile("assets/fonts/Roboto-Regular.ttf", kUiFontEmPx);
    }
    if (fontOk) {
        world.SetUiFont(uiFont);
        auto uiBold = MakeShared<Font>();
        bool boldOk = uiBold->TryLoadTrueTypeFromFile(SPARK_UI_BOLD_FONT_PATH, kUiFontEmPx);
        if (!boldOk) {
            Utf8String boldBuild(SPARK_BUILD_ASSETS_DIR);
            boldBuild.AppendUtf8("/fonts/Roboto-Bold.ttf");
            boldOk = uiBold->TryLoadTrueTypeFromFile(boldBuild.CStr(), kUiFontEmPx);
        }
        if (!boldOk) {
            Utf8String boldSrc(SPARK_ASSETS_DIR);
            boldSrc.AppendUtf8("/fonts/Roboto-Bold.ttf");
            boldOk = uiBold->TryLoadTrueTypeFromFile(boldSrc.CStr(), kUiFontEmPx);
        }
        if (boldOk) {
            world.SetUiBoldFont(uiBold);
        }
    }
}

void FpsGame::SpawnArena(GameWorld& world) {
    unitCube = MakeShared<Mesh>(Utf8String("FpsUnitCube"));
    *unitCube = Mesh::CreateUnitCube();
    world.RegisterMesh(unitCube, "fps_template/unit_cube");

    SharedPtr<Texture2D> groundTex = MakeShared<Texture2D>(Utf8String("FpsGroundTex"));
    const bool usingKenneyGround = DemoAssets::TryLoadGroundDirtTexture(*groundTex);
    if (!usingKenneyGround) {
        *groundTex = Texture2D::CreateCheckerboard(256, 32, Vector3{0.48F, 0.44F, 0.40F}, Vector3{0.32F, 0.36F, 0.28F});
    }
    world.RegisterTexture(groundTex, "fps_template/ground_tex");

    const float groundUvRepeat = usingKenneyGround ? DemoAssets::kKenneyTileWorldUnitsPerRepeat
                                                   : DemoAssets::ProceduralTextureSpanWorldUnits(kSceneGroundHalfExtent);
    groundMesh = MakeShared<Mesh>(Utf8String("FpsGround"));
    *groundMesh = Mesh::CreateGroundPlane(kSceneGroundHalfExtent, groundUvRepeat);
    world.RegisterMesh(groundMesh, "fps_template/ground");

    GameObject* ground = world.CreateGameObject();
    ground->GetName() = Utf8String("Ground");
    ground->AddComponent<TransformComponent>();
    ground->AddComponent<MeshComponent>(groundMesh, Vector3{0.42F, 0.44F, 0.48F});
    if (MaterialComponent* groundMat = ground->AddComponent<MaterialComponent>(groundTex, Vector3::One)) {
        groundMat->SetMetallic(0.04F);
        groundMat->SetRoughness(0.92F);
    }
    roots.PushBack(ground);

    const int kTargets = 8;
    for (int i = 0; i < kTargets; ++i) {
        const float a = static_cast<float>(i) / static_cast<float>(kTargets) * TwoPi;
        const float radius = 7.0F + static_cast<float>(i % 3) * 1.15F;
        const float x = std::cos(a) * radius;
        const float z = std::sin(a) * radius;
        GameObject* t = world.CreateGameObject();
        const std::string name = std::format("Target{}", i);
        t->GetName() = Utf8String(name.c_str());
        if (TransformComponent* tr = t->AddComponent<TransformComponent>()) {
            tr->SetTranslation({x, kCubeScale, z});
            tr->SetUniformScale(kCubeScale);
        }
        t->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, Vector3{0.92F, 0.22F, 0.18F});
        if (MaterialComponent* m = t->AddComponent<MaterialComponent>()) {
            m->SetMetallic(0.12F);
            m->SetRoughness(0.42F);
        }
        targets.PushBack(t);
    }
}

void FpsGame::OnAttach(IEngineContext& context) {
    GameWorld& world = GetWorld();
    MountUiFontIfNeeded(world);
    SpawnArena(world);

    camera.position = {0.0F, 2.2F, 14.0F};
    camera.moveSpeed = 8.0F;
    camera.mouseSensitivity = 0.14F;
    camera.SnapLookAt({0.0F, 1.5F, 0.0F});

    context.GetInput().SetCursorCaptured(true);

    GameObject* hud = world.CreateGameObject();
    hud->GetName() = Utf8String("FpsHud");
    hudText = hud->AddComponent<TextOverlayComponent>();
    hudText->SetScreenPosition(14.0F, 14.0F);
    hudText->SetFontSizePixels(22.0F);
    hudText->SetColor({0.92F, 0.96F, 1.0F});
    hudText->SetText(Utf8String("FPS template — LMB shoot · F1 mouse lock · ESC quit"));
    roots.PushBack(hud);

    glfwSetWindowTitle(context.GetWindow().Handle(), "Spark FPS template");
}

void FpsGame::OnDetach() {
    GameWorld& world = GetWorld();
    DestroyAllTracerBullets();
    for (std::size_t i = 0; i < targets.GetSize(); ++i) {
        if (targets[i] != nullptr) {
            world.DestroyGameObject(targets[i]);
        }
    }
    targets.Clear();
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    hudText = nullptr;
    unitCube.Reset();
    groundMesh.Reset();
}

void FpsGame::CloseWindowIfRequested(IEngineContext& context) const {
    if (context.GetInput().IsKeyPressedThisFrame(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(context.GetWindow().Handle(), GLFW_TRUE);
    }
}

void FpsGame::SpawnTracerBullet(const Vector3& origin, const Vector3& dirUnit) {
    if (!unitCube || tracers.GetSize() >= kMaxTracerBullets) {
        return;
    }
    GameWorld& world = GetWorld();
    GameObject* b = world.CreateGameObject();
    b->GetName() = Utf8String("Tracer");
    if (TransformComponent* tr = b->AddComponent<TransformComponent>()) {
        tr->SetTranslation(origin);
        tr->SetUniformScale(kTracerUniformScale);
    }
    b->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, Vector3{1.0F, 0.92F, 0.35F});
    if (MaterialComponent* m = b->AddComponent<MaterialComponent>()) {
        m->SetMetallic(0.25F);
        m->SetRoughness(0.35F);
        m->SetEmissive({1.0F, 0.78F, 0.12F}, 4.5F);
    }
    TracerBullet tb{};
    tb.go = b;
    tb.velocity = dirUnit * kTracerSpeed;
    tb.timeLeft = kTracerLifetime;
    tracers.PushBack(tb);
}

void FpsGame::UpdateTracerBullets(const float deltaSeconds) {
    GameWorld& world = GetWorld();
    for (std::size_t i = 0; i < tracers.GetSize();) {
        TracerBullet& tr = tracers[i];
        if (tr.go == nullptr) {
            tracers[i] = tracers.GetLast();
            tracers.PopBack();
            continue;
        }
        tr.timeLeft -= deltaSeconds;
        if (tr.timeLeft <= 0.0F) {
            world.DestroyGameObject(tr.go);
            tracers[i] = tracers.GetLast();
            tracers.PopBack();
            continue;
        }
        if (TransformComponent* tc = tr.go->GetComponent<TransformComponent>()) {
            Vector3 p = tc->GetLocalTransform().translation;
            p += tr.velocity * deltaSeconds;
            tc->SetTranslation(p);
        }
        ++i;
    }
}

void FpsGame::DestroyAllTracerBullets() {
    GameWorld& world = GetWorld();
    for (std::size_t i = 0; i < tracers.GetSize(); ++i) {
        if (tracers[i].go != nullptr) {
            world.DestroyGameObject(tracers[i].go);
        }
    }
    tracers.Clear();
}

void FpsGame::TryShootTarget(IEngineContext& context) {
    Spark::IInput& in = context.GetInput();
    if (!in.IsMouseButtonPressedThisFrame(GLFW_MOUSE_BUTTON_LEFT) || !in.IsCursorCaptured()) {
        return;
    }
    ++shotsFired;
    const Vector3 O = camera.position;
    const Vector3 D = camera.Forward();
    SpawnTracerBullet(O + D * kMuzzleForwardOffset, D);

    float bestT = 1.0e9F;
    GameObject* best = nullptr;
    for (std::size_t i = 0; i < targets.GetSize(); ++i) {
        GameObject* g = targets[i];
        if (g == nullptr) {
            continue;
        }
        const Vector3 C = g->GetWorldMatrix().TranslationVector();
        float t = 0.0F;
        if (RaySphereNearestT(O, D, C, kTargetHitRadius, t) && t < bestT) {
            bestT = t;
            best = g;
        }
    }
    if (best == nullptr) {
        return;
    }
    ++hits;
    GetWorld().DestroyGameObject(best);
    for (std::size_t i = 0; i < targets.GetSize(); ++i) {
        if (targets[i] == best) {
            targets[i] = targets.GetLast();
            targets.PopBack();
            break;
        }
    }
}

void FpsGame::OnUpdate(const FrameTiming& timing, IEngineContext& context) {
    sceneTimeSeconds = timing.totalTimeSeconds;
    CloseWindowIfRequested(context);

    Spark::IInput& in = context.GetInput();
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        in.SetCursorCaptured(!in.IsCursorCaptured());
    }
    if (in.IsCursorCaptured()) {
        if (timing.frameIndex > 0U) {
            camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        }
        camera.ProcessMovement(in, timing.deltaTimeSeconds);
    }

    TryShootTarget(context);
    UpdateTracerBullets(timing.deltaTimeSeconds);

    if (hudText != nullptr) {
        const std::string msg = std::format(
                "Targets {} | hits {} | shots {} | tracers {} | F1 lock | LMB shoot | WASD+Space/Shift | ESC quit",
                targets.GetSize(),
                hits,
                shotsFired,
                tracers.GetSize());
        hudText->SetText(Utf8String(msg.c_str()));
    }

    Game::OnUpdate(timing, context);
}

void FpsGame::OnRender(IRenderFrame& /*frame*/, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(72.0F), aspect, 0.1F, 200.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    GetScene().SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);

    const Vector3 forward = camera.Forward();
    Vector3 pr = Vector3::Cross(Vector3{0.0F, 1.0F, 0.0F}, forward).Normalized();
    if (pr.LengthSquared() < 1.0e-8F) {
        pr = Vector3::Cross(Vector3{1.0F, 0.0F, 0.0F}, forward).Normalized();
    }
    const Vector3 pu = Vector3::Cross(forward, pr).Normalized();

    SubmitStandardLitSceneFromWorld(
            GetWorld(),
            context,
            viewProj,
            camera.position,
            Vector3{0.38F, 0.86F, 0.30F}.Normalized(),
            Vector3{1.0F, 0.97F, 0.92F},
            1.05F,
            Vector3{0.10F, 0.11F, 0.14F},
            false,
            pr,
            pu,
            sceneTimeSeconds,
            SceneSpriteSortMode::SortOrderOnly);
}

}  // namespace Spark
