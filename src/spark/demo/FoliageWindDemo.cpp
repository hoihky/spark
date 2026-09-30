#include "spark/demo/FoliageWindDemo.hpp"

#include "spark/config.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/scene/foliage/GrassFoliageAlbedoTexture.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/foliage/FoliageInstancedMeshComponent.hpp"
#include "spark/ecs/components/foliage/WindEnvironmentComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/foliage/GrassBladeMesh.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"
#include "spark/scene/texture/Texture2D.hpp"

#include <GLFW/glfw3.h>

namespace Spark {

namespace {

constexpr float kMeadowHalfExtent = 10.0F;

void SetupDemoCamera(FlyCamera& flyCamera) noexcept {
    flyCamera.moveSpeed = 5.0F;
    flyCamera.position = {kMeadowHalfExtent * 0.55F, 1.85F, kMeadowHalfExtent * 1.05F};
    flyCamera.SnapLookAt({0.0F, 0.35F, 0.0F});
}

}  // namespace

void FoliageWindDemo::Load(GameWorld& world, IEngineContext& context) {
    (void)context;
    roots.Clear();
    sceneTime = 0.0F;
    SetupDemoCamera(camera);

    world.GetWindSubsystem().Reset();

    SharedPtr<Mesh> groundRef = MakeShared<Mesh>(Utf8String("FoliageWindGroundRef"));
    *groundRef = Mesh::CreateGroundPlane(kSceneGroundHalfExtent, kMeadowHalfExtent * 2.0F);
    world.RegisterMesh(groundRef, "spark/demo/foliage_wind_ground");

    GameObject* groundObject = world.CreateGameObject();
    groundObject->GetName() = Utf8String("Ground");
    TransformComponent* groundTr = groundObject->AddComponent<TransformComponent>();
    const float groundScale = kMeadowHalfExtent / kSceneGroundHalfExtent;
    groundTr->SetUniformScale(groundScale);
    groundObject->AddComponent<MeshComponent>(groundRef, SceneMeshSlot::GroundPlane, Vector3{0.32F, 0.38F, 0.28F});
    if (MaterialComponent* groundMat = groundObject->AddComponent<MaterialComponent>()) {
        groundMat->SetTint({0.32F, 0.38F, 0.28F});
        groundMat->SetRoughness(0.92F);
    }
    roots.PushBack(groundObject);

    windEnvironmentObject = world.CreateGameObject();
    windEnvironmentObject->GetName() = Utf8String("WindEnvironment");
    windEnvironmentObject->AddComponent<TransformComponent>();
    WindEnvironmentComponent* windEnv = windEnvironmentObject->AddComponent<WindEnvironmentComponent>();
    WindSettings settings{};
    settings.SetEnabled(true);
    settings.SetDirectionWorld({0.85F, 0.0F, 0.35F});
    settings.SetBaseSpeedMetersPerSecond(1.25F);
    settings.SetGustAmplitude(0.45F);
    settings.SetGustFrequencyHertz(0.22F);
    settings.SetTurbulence(0.28F);
    windEnv->GetSettings() = settings;
    roots.PushBack(windEnvironmentObject);

    SharedPtr<Mesh> bladeMesh = GrassBladeMesh::CreateSharedBladeMesh();
    world.RegisterMesh(bladeMesh, "spark/demo/grass_blade");

    SharedPtr<Texture2D> grassTex = GrassFoliageAlbedoTexture::CreateSharedBladeAlbedo();
    world.RegisterTexture(grassTex, "spark/demo/foliage_wind_grass_albedo");

    GameObject* field = world.CreateGameObject();
    field->GetName() = Utf8String("GrassField");
    TransformComponent* fieldTr = field->AddComponent<TransformComponent>();
    fieldTr->SetTranslation({0.0F, 0.0F, 0.0F});
    FoliageInstancedMeshComponent* foliage = field->AddComponent<FoliageInstancedMeshComponent>();
    foliage->SetBladeMesh(bladeMesh);
    foliage->SetAlbedoTexture(grassTex);
    foliage->SetAlbedoTint({0.52F, 0.92F, 0.38F});
    foliage->SetWindBendScale(0.26F);
    foliage->SetAlphaCutoff(0.42F);
    foliage->ConfigureGridWithinSquare(kMeadowHalfExtent, 0.2F, 0.45F);
    roots.PushBack(field);

    helpHud.Mount(world, "Foliage wind meadow");
    helpHud.SetControlHints("W toggle wind | +/- wind speed | F1 mouse | WASD fly");
    helpHud.SetDetail("GPU instanced grass blades · global wind UBO");
    context.GetInput().SetCursorCaptured(true);
}

void FoliageWindDemo::Unload(GameWorld& world) {
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    windEnvironmentObject = nullptr;
    world.GetWindSubsystem().Reset();
    helpHud.Unmount(world);
}

void FoliageWindDemo::Simulate(const FrameTiming& timing, IEngineContext& context) {
    sceneTime += timing.deltaTimeSeconds;
    IInput& input = context.GetInput();
    if (input.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        input.SetCursorCaptured(!input.IsCursorCaptured());
    }
    camera.ProcessMovement(input, timing.deltaTimeSeconds);
    const float mouseDx = input.GetMouseDeltaX();
    const float mouseDy = input.GetMouseDeltaY();
    if (input.IsCursorCaptured()) {
        camera.AddLook(mouseDx, mouseDy);
    }

    if (windEnvironmentObject != nullptr) {
        WindEnvironmentComponent* wind = windEnvironmentObject->GetComponent<WindEnvironmentComponent>();
        if (wind != nullptr) {
            WindSettings& settings = wind->GetSettings();
            if (input.IsKeyPressedThisFrame(GLFW_KEY_W)) {
                settings.SetEnabled(!settings.IsEnabled());
            }
            if (input.IsKeyDown(GLFW_KEY_EQUAL) || input.IsKeyDown(GLFW_KEY_KP_ADD)) {
                settings.SetBaseSpeedMetersPerSecond(
                        settings.GetBaseSpeedMetersPerSecond() + 0.9F * timing.deltaTimeSeconds);
            }
            if (input.IsKeyDown(GLFW_KEY_MINUS) || input.IsKeyDown(GLFW_KEY_KP_SUBTRACT)) {
                const float next = settings.GetBaseSpeedMetersPerSecond() - 0.9F * timing.deltaTimeSeconds;
                settings.SetBaseSpeedMetersPerSecond(next > 0.0F ? next : 0.0F);
            }
        }
    }
    helpHud.Update(timing, context);
}

void FoliageWindDemo::Render(Scene& /*scene*/, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.08F, 240.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    LitSceneSubmitOptions options{};
    options.lightDirectionWorld = Vector3{0.42F, 0.86F, 0.28F}.Normalized();
    options.lightColor = {1.0F, 0.98F, 0.92F};
    options.lightIntensity = 1.0F;
    options.ambientColor = {0.08F, 0.1F, 0.12F};
    options.enableParticles = false;
    options.sceneTimeSeconds = sceneTime;

    SceneRenderParams params{};
    FillStandardLitSceneFromWorld(world, context, viewProj, camera.position, options, params);
    params.lightingProfile = SceneLightingProfile::Outdoor;
    params.worldClearColorEnabled = true;
    params.worldClearColor = {0.55F, 0.74F, 0.95F};
    params.directionalShadowsEnabled = true;
    params.ssaoEnabled = true;

    helpHud.PatchSceneRenderParams(params, world);
    params.Sanitize();
    context.SetSceneRenderParams(params);
}

}  // namespace Spark
