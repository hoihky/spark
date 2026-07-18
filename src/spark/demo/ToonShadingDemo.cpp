#include "spark/demo/ToonShadingDemo.hpp"

#include "spark/scene/SceneSubmit.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Quaternion.hpp"

#include <algorithm>

namespace Spark {

void ToonShadingDemo::Load(GameWorld& w, IEngineContext& /*context*/) {
    roots.Clear();
    roots.Reserve(12);

    unitCube = MakeShared<Mesh>(Utf8String("ToonDemoCube"));
    *unitCube = Mesh::CreateUnitCube();
    w.RegisterMesh(unitCube, "spark/demo/toon_cube");

    groundMesh = MakeShared<Mesh>(Utf8String("ToonDemoGround"));
    *groundMesh = Mesh::CreateGroundPlane(kSceneGroundHalfExtent);
    w.RegisterMesh(groundMesh, "spark/demo/toon_ground");

    GameObject* ground = w.CreateGameObject();
    ground->GetName() = Utf8String("Ground");
    ground->AddComponent<TransformComponent>();
    ground->AddComponent<MeshComponent>(
            groundMesh, SceneMeshSlot::GroundPlane, Vector3{0.42F, 0.46F, 0.5F});
    if (MaterialComponent* m = ground->AddComponent<MaterialComponent>()) {
        m->SetShadingModel(SceneShadingModel::LitPbr);
        m->SetMetallic(0.0F);
        m->SetRoughness(0.75F);
    }
    roots.PushBack(ground);

    auto addCube = [&](float x, float z, Vector3 albedo, SceneShadingModel shade, int bands, float rimI, float rimP,
                       float scaleY = 1.0F) {
        GameObject* go = w.CreateGameObject();
        go->GetName() = Utf8String("Shape");
        if (TransformComponent* tr = go->AddComponent<TransformComponent>()) {
            tr->SetTranslation({x, kCubeScale * scaleY, z});
            tr->SetScale({0.95F, kCubeScale * scaleY, 0.95F});
        }
        go->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, albedo);
        if (MaterialComponent* m = go->AddComponent<MaterialComponent>()) {
            m->SetShadingModel(shade);
            m->SetMetallic(shade == SceneShadingModel::ToonCel ? 0.12F : 0.35F);
            m->SetRoughness(shade == SceneShadingModel::ToonCel ? 0.38F : 0.28F);
            if (shade == SceneShadingModel::ToonCel) {
                m->SetToonDiffuseBands(bands);
                m->SetToonRimIntensity(rimI);
                m->SetToonRimPower(rimP);
            }
        }
        roots.PushBack(go);
        return go;
    };

    // Reference PBR column (left)
    addCube(-3.2F, 0.0F, Vector3{0.75F, 0.38F, 0.32F}, SceneShadingModel::LitPbr, 3, 0.35F, 4.0F);
    addCube(-3.2F, 2.5F, Vector3{0.35F, 0.62F, 0.88F}, SceneShadingModel::LitPbr, 3, 0.35F, 4.0F);

    // Toon column (center) — front cube's bands are adjustable at runtime
    addCube(0.0F, 0.0F, Vector3{0.82F, 0.55F, 0.28F}, SceneShadingModel::ToonCel, 3, 0.42F, 3.5F);
    toonBandsDemoCube = addCube(0.0F, 2.5F, Vector3{0.38F, 0.82F, 0.48F}, SceneShadingModel::ToonCel, 3, 0.55F, 5.0F);
    if (toonBandsDemoCube != nullptr) {
        toonBandsMaterial = toonBandsDemoCube->GetComponent<MaterialComponent>();
    }

    // Stronger cel + rim (right)
    addCube(3.2F, 0.0F, Vector3{0.55F, 0.42F, 0.92F}, SceneShadingModel::ToonCel, 2, 0.62F, 2.8F, 1.15F);
    addCube(3.2F, 2.5F, Vector3{0.92F, 0.5F, 0.38F}, SceneShadingModel::ToonCel, 6, 0.28F, 6.0F);

    GameObject* plGo = w.CreateGameObject();
    plGo->GetName() = Utf8String("FillLight");
    if (TransformComponent* tr = plGo->AddComponent<TransformComponent>()) {
        tr->SetTranslation({-1.2F, 3.4F, 3.8F});
    }
    plGo->AddComponent<PointLightComponent>(Vector3{0.75F, 0.55F, 1.0F}, 2.4F, 14.0F)->SetCastsShadow(true);
    roots.PushBack(plGo);

    GameObject* spotGo = w.CreateGameObject();
    spotGo->GetName() = Utf8String("KeySpot");
    const Vector3 spotPos{3.8F, 5.2F, 9.5F};
    const Vector3 spotTarget{0.3F, 1.0F, 0.0F};
    if (TransformComponent* tr = spotGo->AddComponent<TransformComponent>()) {
        tr->SetTranslation(spotPos);
        const Vector3 dir = (spotTarget - spotPos).Normalized();
        tr->SetRotation(Quaternion::FromShortestArc(Vector3{0.0F, 0.0F, -1.0F}, dir));
    }
    spotGo->AddComponent<SpotLightComponent>(Vector3{1.0F, 0.92F, 0.78F}, 5.5F, 22.0F, 22.0F, 38.0F)
            ->SetCastsShadow(true);
    roots.PushBack(spotGo);

    camera.position = {0.0F, 4.8F, 14.5F};
    camera.SnapLookAt({0.0F, 1.2F, 0.0F});

    helpHud = w.CreateGameObject();
    helpHud->GetName() = Utf8String("ToonHelpHud");
    helpText = helpHud->AddComponent<TextOverlayComponent>();
    helpText->SetScreenPosition(12.0F, 12.0F);
    helpText->SetFontSizePixels(18.0F);
    helpText->SetColor({0.88F, 0.92F, 0.98F});
    helpText->SetText(Utf8String("…"));
    roots.PushBack(helpHud);
}

void ToonShadingDemo::Unload(GameWorld& w) {
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    helpHud = nullptr;
    helpText = nullptr;
    toonBandsDemoCube = nullptr;
    toonBandsMaterial = nullptr;
}

void ToonShadingDemo::Simulate(const FrameTiming& timing, IEngineContext& context) {
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

    if (toonBandsMaterial != nullptr) {
        int b = toonBandsMaterial->GetToonDiffuseBands();
        if (in.IsKeyPressedThisFrame(GLFW_KEY_LEFT_BRACKET)) {
            b = std::max(2, b - 1);
            toonBandsMaterial->SetToonDiffuseBands(b);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_RIGHT_BRACKET)) {
            b = std::min(8, b + 1);
            toonBandsMaterial->SetToonDiffuseBands(b);
        }
    }

    spinRadians += timing.deltaTimeSeconds * 0.55F;
    if (toonBandsDemoCube != nullptr) {
        if (TransformComponent* tr = toonBandsDemoCube->GetComponent<TransformComponent>()) {
            const Vector3 t = tr->GetLocalTransform().translation;
            tr->SetTranslation({t.x, kCubeScale, t.z});
            tr->SetRotation(Quaternion::FromAxisAngle(Vector3{0.0F, 1.0F, 0.0F}, spinRadians));
        }
    }

    if (helpText != nullptr) {
        const int bands = toonBandsMaterial != nullptr ? toonBandsMaterial->GetToonDiffuseBands() : 3;
        helpText->SetText(Utf8String(
                std::format(
                        "Toon / cel shading — Lit PBR (left) vs ToonCel (center/right) on MaterialComponent\n"
                        "  F1 mouse · WASD fly · [ ] bands on green toon cube ({} levels) · ESC menu",
                        bands)
                        .c_str()));
    }
}

void ToonShadingDemo::Render(Scene& scene, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(58.0F), aspect, 0.1F, 220.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    scene.SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);

    Vector3 forward = camera.Forward();
    Vector3 pr = Vector3::Cross(Vector3{0.0F, 1.0F, 0.0F}, forward).Normalized();
    if (pr.LengthSquared() < 1e-8F) {
        pr = Vector3::Cross(Vector3{1.0F, 0.0F, 0.0F}, forward).Normalized();
    }
    const Vector3 pu = Vector3::Cross(forward, pr).Normalized();

    SubmitStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Vector3{0.38F, 0.86F, 0.28F}.Normalized(),
            Vector3{1.0F, 0.97F, 0.92F},
            1.25F,
            Vector3{0.1F, 0.11F, 0.14F},
            false,
            pr,
            pu,
            0.0F,
            SceneSpriteSortMode::SortOrderOnly);
}

}  // namespace Spark
