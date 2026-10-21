#include "spark/demo/PhysicsShowcase3DDemo.hpp"

#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsAttachedBody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/physics/CollisionFilter3D.hpp"
#include "spark/physics/PhysicsBodyHandoff3D.hpp"
#include "spark/scene/query/ScenePhysicsRaycast3D.hpp"
#include "spark/scene/query/SceneRaycast.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

#include <cmath>
#include <format>

namespace Spark {

namespace {

Vector3 FlattenYawForward(const float yawRadians) noexcept {
    return {std::sin(yawRadians), 0.0F, -std::cos(yawRadians)};
}

Vector3 FlattenYawRight(const float yawRadians) noexcept {
    const Vector3 forward = FlattenYawForward(yawRadians);
    Vector3 right = Vector3::Cross(forward, Vector3::UnitY);
    if (right.LengthSquared() < 1.0e-8F) {
        return Vector3::UnitX;
    }
    return right.Normalized();
}

constexpr float kGroundSurfaceY = 0.36F;

}  // namespace

void PhysicsShowcase3DDemo::Load(GameWorld& w, IEngineContext& context) {
    physics = PhysicsSubsystem{};
    roots.Clear();
    projectiles.Clear();
    player = nullptr;
    playerController = nullptr;
    playerTransform = nullptr;
    crate = nullptr;
    carrierAnchor = nullptr;
    goalTrigger = nullptr;
    goalStayTicks = 0;
    lastPickFrame = -1;
    lastPickName.Clear();
    useFixedStep = true;
    recordingReplay = false;
    camera = FlyCamera{};

    skyMesh = MakeShared<Mesh>(Utf8String("PhysShowSky"));
    *skyMesh = Mesh::CreateSkySphere(1.0F, 16, 32);
    groundMesh = MakeShared<Mesh>(Utf8String("PhysShowGround"));
    *groundMesh = Mesh::CreateGroundPlane(kSceneGroundHalfExtent);
    cubeMesh = MakeShared<Mesh>(Utf8String("PhysShowCube"));
    *cubeMesh = Mesh::CreateUnitCube();
    ballMesh = MakeShared<Mesh>(Utf8String("PhysShowBall"));
    *ballMesh = Mesh::CreateSkySphere(0.18F, 10, 18);

    GameObject* sky = w.CreateGameObject();
    sky->GetName() = Utf8String("PhysShowSky");
    sky->AddComponent<TransformComponent>()->SetUniformScale(96.0F);
    SkyComponent* skyComp = sky->AddComponent<SkyComponent>(SceneSkyMode::Box);
    skyComp->SetTint({0.38F, 0.52F, 0.78F});
    sky->AddComponent<MeshComponent>(skyMesh, SceneMeshSlot::Custom, Vector3{0.32F, 0.45F, 0.68F});
    roots.PushBack(sky);

    GameObject* ground = w.CreateGameObject();
    ground->GetName() = Utf8String("PhysShowGround");
    ground->AddComponent<TransformComponent>();
    ground->AddComponent<MeshComponent>(
            groundMesh, SceneMeshSlot::GroundPlane, Vector3{0.42F, 0.48F, 0.38F});
    {
        const float hx = kSceneGroundHalfExtent;
        const float hy = 0.18F;
        auto* box = ground->AddComponent<BoxCollider3DComponent>(Vector3{hx, hy, hx}, Vector3{0.0F, hy, 0.0F});
        box->SetCategoryBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Environment));
        box->SetMaskBits(CollisionFilter3D::AllLayersMask());
        ground->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Static, 1.0F);
        ground->AddComponent<PhysicsMaterial3DComponent>(0.78F, 0.68F, 0.35F);
    }
    roots.PushBack(ground);

    GameObject* thinWall = w.CreateGameObject();
    thinWall->GetName() = Utf8String("PhysShowCcdWall");
    TransformComponent* wallTr = thinWall->AddComponent<TransformComponent>();
    wallTr->SetTranslation({6.0F, 1.2F, 10.0F});
    wallTr->SetScale({6.0F, 2.4F, 0.06F});
    thinWall->AddComponent<MeshComponent>(cubeMesh, SceneMeshSlot::UnitCube, Vector3{0.55F, 0.62F, 0.7F});
    auto* wallBox = thinWall->AddComponent<BoxCollider3DComponent>(Vector3{3.0F, 1.2F, 0.03F}, Vector3::Zero);
    wallBox->SetCategoryBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Environment));
    wallBox->SetMaskBits(CollisionFilter3D::AllLayersMask());
    thinWall->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Static, 1.0F);
    roots.PushBack(thinWall);

    GameObject* goal = w.CreateGameObject();
    goal->GetName() = Utf8String("PhysShowGoal");
    TransformComponent* goalTr = goal->AddComponent<TransformComponent>();
    goalTr->SetTranslation({0.0F, 1.5F, 14.0F});
    goal->AddComponent<MeshComponent>(cubeMesh, SceneMeshSlot::UnitCube, Vector3{0.25F, 0.85F, 0.35F});
    goalTr->SetScale({8.0F, 3.0F, 4.0F});
    goalTrigger = goal->AddComponent<TriggerVolume3DComponent>(
            TriggerVolume3DShape::Box, Vector3{4.0F, 1.5F, 2.0F}, Vector3::Zero);
    goalTrigger->SetCategoryBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Trigger));
    goalTrigger->SetMaskBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Player));
    roots.PushBack(goal);

    player = w.CreateGameObject();
    player->GetName() = Utf8String("PhysShowPlayer");
    playerTransform = player->AddComponent<TransformComponent>();
    playerTransform->SetTranslation({0.0F, 0.55F, -4.0F});
    player->AddComponent<MeshComponent>(ballMesh, SceneMeshSlot::Custom, Vector3{0.35F, 0.72F, 0.95F});
    playerController = player->AddComponent<CharacterController3DComponent>(0.38F, Vector3{0.0F, 0.92F, 0.0F});
    playerController->SetStepOffset(0.22F);
    playerController->SetCategoryBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Player));
    playerController->SetMaskBits(
            CollisionFilter3D::LayerBit(PhysicsLayers3D::Environment)
            | CollisionFilter3D::LayerBit(PhysicsLayers3D::Enemy));
    roots.PushBack(player);

    carrierAnchor = w.CreateGameObject();
    carrierAnchor->GetName() = Utf8String("PhysShowCarryAnchor");
    carrierAnchor->AddComponent<TransformComponent>();
    roots.PushBack(carrierAnchor);

    crate = w.CreateGameObject();
    crate->GetName() = Utf8String("PhysShowCrate");
    TransformComponent* crateTr = crate->AddComponent<TransformComponent>();
    constexpr float kCrateScale = 0.95F;
    constexpr float kCrateSphereRadius = 0.46F;
    const float kCrateCenterY = kGroundSurfaceY + kCrateSphereRadius * kCrateScale;
    crateTr->SetTranslation({2.2F, kCrateCenterY, -2.0F});
    crate->AddComponent<MeshComponent>(cubeMesh, SceneMeshSlot::UnitCube, Vector3{0.78F, 0.55F, 0.32F});
    crateTr->SetUniformScale(kCrateScale);
    // Sphere approximates the box for dynamic simulation (engine integrates sphere/capsule only).
    auto* crateSphere = crate->AddComponent<SphereCollider3DComponent>(kCrateSphereRadius, Vector3::Zero);
    crateSphere->SetCategoryBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Enemy));
    crateSphere->SetMaskBits(CollisionFilter3D::AllLayersMask());
    auto* crateRb = crate->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Dynamic, 1.0F);
    crateRb->SetInverseMass(1.0F / 4.0F);
    crateRb->SetLinearDamping(0.14F);
    crateRb->SetAngularDamping(4.5F);
    crateRb->SetRestitution(0.22F);
    crate->AddComponent<PhysicsMaterial3DComponent>(0.72F, 0.62F, 0.22F);
    crate->AddComponent<PhysicsAttachedBody3DComponent>();
    roots.PushBack(crate);

    GameObject* blocker = w.CreateGameObject();
    blocker->GetName() = Utf8String("PhysShowBlocker");
    TransformComponent* blockTr = blocker->AddComponent<TransformComponent>();
    blockTr->SetTranslation({7.5F, 1.1F, 2.0F});
    blocker->AddComponent<MeshComponent>(cubeMesh, SceneMeshSlot::UnitCube, Vector3{0.62F, 0.4F, 0.48F});
    blockTr->SetUniformScale(1.1F);
    auto* blockCapsule = blocker->AddComponent<CapsuleCollider3DComponent>(0.55F, 1.6F, CapsuleDirection3D::Y, Vector3::Zero);
    blockCapsule->SetCategoryBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Enemy));
    blockCapsule->SetMaskBits(CollisionFilter3D::AllLayersMask());
    auto* blockRb = blocker->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Dynamic, 1.0F);
    blockRb->SetInverseMass(1.0F / 80.0F);
    blockRb->SetLinearDamping(0.08F);
    blockRb->SetAngularDamping(1.5F);
    roots.PushBack(blocker);

    GameObject* light = w.CreateGameObject();
    light->GetName() = Utf8String("PhysShowSun");
    light->AddComponent<TransformComponent>()->SetTranslation({14.0F, 24.0F, 10.0F});
    PointLightComponent* pl = light->AddComponent<PointLightComponent>();
    pl->SetRange(100.0F);
    pl->SetColor({1.0F, 0.96F, 0.88F});
    pl->SetIntensity(2.2F);
    pl->SetCastsShadow(true);
    roots.PushBack(light);

    PhysicsWorld3DSettings& settings = physics.GetWorld3D().GetSettings();
    settings.gravityY = -18.0F;
    settings.resolveIterations = 10;
    settings.substeps = 2;
    settings.sweptStaticCcdBinaryIterations = 8;
    settings.sweptDynamicCcdBinaryIterations = 6;
    settings.fixedSimulationTimestep = 1.0F / 60.0F;
    settings.maxFixedSimulationStepsPerFrame = 6;
    physics.GetWorld3D().GetContactBroadcaster().SetEnabled(true);
    CharacterController3DSettings& ccSettings = physics.GetCharacterController3D().GetSettings();
    ccSettings.gravityY = settings.gravityY;
    ccSettings.maxFallSpeed = settings.maxFallSpeed;
    ccSettings.broadPhaseCellSize = physics.GetWorld3D().GetBroadPhaseCellSize();
    physics.GetWorld3D().ResetFixedStepAccumulator();
    physics.GetQueries3D().SetCellWorldSize(physics.GetWorld3D().GetBroadPhaseCellSize());
    physics.GetQueries3D().RebuildStatics(w);
    physics.GetQueries3D().RebuildDynamics(w);

    helpHud.Mount(w, "Physics 3D showcase");
    helpHud.SetControlHints(
            "WASD move · E carry crate · F fast shot (CCD) · LMB pick · F1 look · F2 fixed step · F3 pause · "
            "F4 record · F5 replay");

    camera.position = {0.0F, 3.5F, -10.0F};
    camera.SnapLookAt({0.0F, 1.0F, 4.0F});
    camera.moveSpeed = 0.0F;
    camera.mouseSensitivity = 0.12F;

    MountUiFont(w);
    context.GetInput().SetCursorCaptured(true);
    UpdateHelpDetail();
}

void PhysicsShowcase3DDemo::Unload(GameWorld& w) {
    helpHud.Unmount(w);
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            w.DestroyGameObject(roots[i]);
        }
    }
    for (std::size_t i = 0; i < projectiles.GetSize(); ++i) {
        if (projectiles[i] != nullptr) {
            w.DestroyGameObject(projectiles[i]);
        }
    }
    roots.Clear();
    projectiles.Clear();
    player = nullptr;
    playerController = nullptr;
    playerTransform = nullptr;
    crate = nullptr;
    carrierAnchor = nullptr;
    goalTrigger = nullptr;
    physics = PhysicsSubsystem{};
}

void PhysicsShowcase3DDemo::SpawnFastProjectile(GameWorld& world) {
    Vector3 dir = FlattenYawForward(camera.yaw);
    dir.y = 0.1F;
    if (dir.LengthSquared() < 1.0e-6F) {
        return;
    }
    dir = dir.Normalized();

    Vector3 origin = camera.position;
    if (playerTransform != nullptr) {
        const Vector3 playerPos = playerTransform->GetLocalTransform().translation;
        origin = playerPos + Vector3{0.0F, 1.45F, 0.0F};
    }

    float spawnOffset = 0.85F;
    PhysicsQueryFilter3D filter{};
    filter.queryCategoryBits = CollisionFilter3D::DefaultCategory();
    filter.queryMaskBits = CollisionFilter3D::AllLayersMask();
    PhysicsRaycastHit3D blockHit{};
    if (physics.GetQueries3D().Raycast(world, origin, dir, 1.25F, filter, player, blockHit)) {
        if (blockHit.distanceAlongRay < 0.28F) {
            return;
        }
        spawnOffset = std::min(blockHit.distanceAlongRay - 0.18F, 0.85F);
    }
    const Vector3 spawn = origin + dir * spawnOffset;

    while (projectiles.GetSize() > 6) {
        GameObject* old = projectiles[0];
        if (old != nullptr) {
            world.DestroyGameObject(old);
        }
        projectiles.RemoveAt(0);
    }

    GameObject* shot = world.CreateGameObject();
    shot->GetName() = Utf8String("PhysShowProjectile");
    shot->AddComponent<TransformComponent>()->SetTranslation(spawn);
    shot->AddComponent<MeshComponent>(ballMesh, SceneMeshSlot::Custom, Vector3{0.95F, 0.25F, 0.2F});
    auto* shotSphere = shot->AddComponent<SphereCollider3DComponent>(0.12F, Vector3::Zero);
    shotSphere->SetCategoryBits(CollisionFilter3D::LayerBit(PhysicsLayers3D::Projectile));
    shotSphere->SetMaskBits(
            CollisionFilter3D::LayerBit(PhysicsLayers3D::Environment)
            | CollisionFilter3D::LayerBit(PhysicsLayers3D::Enemy));
    auto* rb = shot->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Dynamic, 1.0F);
    rb->SetInverseMass(1.0F / 0.28F);
    rb->SetLinearDamping(0.06F);
    rb->SetAngularDamping(1.2F);
    rb->SetRestitution(0.48F);
    shot->AddComponent<PhysicsMaterial3DComponent>(0.42F, 0.38F, 0.48F);
    rb->SetVelocity(dir * 34.0F);
    rb->SetAngularVelocity(Vector3::Zero);
    projectiles.PushBack(shot);
    physics.GetQueries3D().RebuildDynamics(world);
}

void PhysicsShowcase3DDemo::UpdateCarrierAnchor() noexcept {
    if (playerTransform == nullptr || carrierAnchor == nullptr) {
        return;
    }
    const Vector3 playerPos = playerTransform->GetLocalTransform().translation;
    const Vector3 forward = FlattenYawForward(camera.yaw);
    carrierAnchor->GetComponent<TransformComponent>()->SetTranslation(
            playerPos + Vector3::UnitY * 1.35F + forward * 0.85F);
}

void PhysicsShowcase3DDemo::TryToggleCarry(GameWorld& world) {
    if (crate == nullptr || carrierAnchor == nullptr || playerTransform == nullptr) {
        return;
    }
    auto* attached = crate->GetComponent<PhysicsAttachedBody3DComponent>();
    if (attached != nullptr && attached->IsAttached()) {
        const Vector3 impulse = FlattenYawForward(camera.yaw) * 7.0F + Vector3::UnitY * 2.8F;
        PhysicsBodyHandoff3D::ReleaseRigidbody(*crate, impulse);
        if (auto* crateRb = crate->GetComponent<Rigidbody3DComponent>()) {
            crateRb->SetAngularVelocity(Vector3::Zero);
        }
        if (TransformComponent* crateTr = crate->GetComponent<TransformComponent>()) {
            crateTr->SetRotation(Quaternion::Identity);
        }
        physics.GetQueries3D().RebuildDynamics(world);
        return;
    }
    const Vector3 playerPos = playerTransform->GetLocalTransform().translation;
    const Vector3 cratePos = crate->GetComponent<TransformComponent>()->GetLocalTransform().translation;
    const Vector3 delta = cratePos - playerPos;
    if (delta.LengthSquared() > 3.5F * 3.5F) {
        return;
    }
    UpdateCarrierAnchor();
    if (TransformComponent* crateTr = crate->GetComponent<TransformComponent>()) {
        crateTr->SetRotation(Quaternion::Identity);
    }
    if (auto* crateRb = crate->GetComponent<Rigidbody3DComponent>()) {
        crateRb->SetAngularVelocity(Vector3::Zero);
        crateRb->SetVelocity(Vector3::Zero);
    }
    PhysicsBodyHandoff3D::AttachRigidbodyToCarrier(world, *crate, *carrierAnchor, Vector3::Zero);
    physics.GetQueries3D().RebuildDynamics(world);
}

void PhysicsShowcase3DDemo::TryRaycastPick(
        IEngineContext& context,
        GameWorld& world,
        Scene& scene,
        const int frameIndex) {
    IInput& in = context.GetInput();
    if (!in.IsMouseButtonPressedThisFrame(0)) {
        return;
    }
    ScenePhysicsRaycast3D unified{scene};
    unified.SetPhysicsQueries(&physics.GetQueries3D());

    SceneRay ray{};
    ray.origin = camera.position;
    ray.direction = camera.Forward().Normalized();
    ray.tMax = 80.0F;

    SceneRaycastHit hit{};
    SceneRaycastOptions sceneOpts{};
    sceneOpts.pickRigidMeshes = true;
    sceneOpts.pickSkinnedMeshes = false;

    PhysicsQueryFilter3D filter{};
    filter.queryCategoryBits = CollisionFilter3D::DefaultCategory();
    filter.queryMaskBits = CollisionFilter3D::AllLayersMask();

    if (!unified.RaycastClosest(world, ray, hit, sceneOpts, filter, player)) {
        lastPickName = Utf8String("(miss)");
    } else if (hit.object != nullptr) {
        lastPickName = hit.object->GetName();
    } else {
        lastPickName = Utf8String("(hit)");
    }
    lastPickFrame = frameIndex;
}

void PhysicsShowcase3DDemo::UpdateHelpDetail() {
    const char* stepMode = useFixedStep ? "fixed 60 Hz" : "variable dt";
    const char* pause =
            physics.GetSimulationDomain3D().GetAdvanceMode() == PhysicsSimulationDomain3D::AdvanceMode::Paused
                    ? "paused"
                    : "live";
    const char* rec = recordingReplay ? "on" : "off";
    helpDetailLine = Utf8String(std::format(
            "goal stay {} | pick {} (frame {}) | sim {} | {} | record {}",
            goalStayTicks,
            lastPickName.CStr(),
            lastPickFrame,
            stepMode,
            pause,
            rec).c_str());
    helpHud.SetDetail(helpDetailLine.CStr());
}

void PhysicsShowcase3DDemo::Simulate(
        const FrameTiming& timing,
        IEngineContext& context,
        GameWorld& world,
        Scene& scene) {
    IInput& in = context.GetInput();

    if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        in.SetCursorCaptured(!in.IsCursorCaptured());
    }
    if (in.IsCursorCaptured() && timing.frameIndex > 0U) {
        camera.AddLook(in.GetMouseDeltaX(), 0.0F);
    }

    if (in.IsKeyPressedThisFrame(GLFW_KEY_F2)) {
        useFixedStep = !useFixedStep;
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F3)) {
        auto& domain = physics.GetSimulationDomain3D();
        domain.SetAdvanceMode(
                domain.GetAdvanceMode() == PhysicsSimulationDomain3D::AdvanceMode::Paused
                        ? PhysicsSimulationDomain3D::AdvanceMode::Live
                        : PhysicsSimulationDomain3D::AdvanceMode::Paused);
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F4)) {
        recordingReplay = !recordingReplay;
        physics.GetSimulationDomain3D().SetRecordingEnabled(recordingReplay);
        if (recordingReplay) {
            physics.GetSimulationDomain3D().GetReplayBuffer().SetCapacity(120);
        }
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F5)) {
        physics.PlaybackPhysics3DFrame(world, 0);
    }

    if (in.IsKeyPressedThisFrame(GLFW_KEY_E)) {
        TryToggleCarry(world);
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_F)) {
        SpawnFastProjectile(world);
    }

    for (std::size_t pi = projectiles.GetSize(); pi > 0; --pi) {
        GameObject* proj = projectiles[pi - 1];
        if (proj == nullptr || proj->IsPendingDestroy()) {
            projectiles.RemoveAt(pi - 1);
            continue;
        }
        const TransformComponent* tr = proj->GetComponent<TransformComponent>();
        if (tr != nullptr) {
            const Vector3 p = tr->GetLocalTransform().translation;
            if (p.y < -4.0F || p.LengthSquared() > 120.0F * 120.0F) {
                world.DestroyGameObject(proj);
                projectiles.RemoveAt(pi - 1);
            }
        }
    }
    TryRaycastPick(context, world, scene, static_cast<int>(timing.frameIndex));

    if (playerController != nullptr) {
        float moveX = 0.0F;
        float moveZ = 0.0F;
        if (in.IsKeyDown(GLFW_KEY_W)) {
            moveZ += 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_S)) {
            moveZ -= 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_A)) {
            moveX -= 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_D)) {
            moveX += 1.0F;
        }
        const Vector3 f = FlattenYawForward(camera.yaw);
        const Vector3 r = FlattenYawRight(camera.yaw);
        const Vector3 wish = (f * moveZ + r * moveX);
        const float wishLen = wish.Length();
        Vector3 vel = wish;
        if (wishLen > 1.0e-4F) {
            vel = vel * (6.0F / wishLen);
        }
        playerController->SetMoveInput(vel);
    }

    if (crate != nullptr) {
        const auto* attached = crate->GetComponent<PhysicsAttachedBody3DComponent>();
        if (attached != nullptr && attached->IsAttached()) {
            UpdateCarrierAnchor();
        }
    }

    PhysicsWorld3DSettings& settings = physics.GetWorld3D().GetSettings();
    settings.fixedSimulationTimestep = useFixedStep ? (1.0F / 60.0F) : 0.0F;

    physics.SimulateAll3D(world, timing);

    if (goalTrigger != nullptr && player != nullptr && goalTrigger->IsOverlapping(*player)) {
        ++goalStayTicks;
    }

    if (playerTransform != nullptr) {
        const Vector3 p = playerTransform->GetLocalTransform().translation;
        constexpr float kCamDist = 7.5F;
        constexpr float kCamHeight = 2.8F;
        const Vector3 back = FlattenYawForward(camera.yaw) * -kCamDist;
        camera.position = p + Vector3{back.x, kCamHeight, back.z};
        camera.SnapLookAt(p + Vector3{0.0F, 1.1F, 0.0F});
    }

    UpdateHelpDetail();
    helpHud.Update(timing, context);
}

void PhysicsShowcase3DDemo::Render(Scene& scene, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.1F, 320.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    SceneRenderParams params{};
    params.viewProjection = viewProj;
    params.cameraPositionWorld = camera.position;
    params.lightDirectionWorld = Vector3{0.35F, 0.88F, 0.32F}.Normalized();
    params.lightColor = {1.0F, 0.96F, 0.9F};
    params.lightIntensity = 0.95F;
    params.ambientColor = {0.09F, 0.11F, 0.14F};

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
    params.draws.Reserve(32);

    scene.ForEachPointLight([&params](const PointLightComponent& pl, const Matrix4& worldMat) {
        if (params.pointLights.GetSize() >= SceneRenderParams::MaxPointLights) {
            return;
        }
        ScenePointLight gpu{};
        gpu.positionWorld = worldMat.TranslationVector();
        gpu.range = pl.GetRange();
        gpu.color = pl.GetColor();
        gpu.intensity = pl.GetIntensity();
        gpu.castsShadow = pl.CastsShadow();
        params.pointLights.PushBack(gpu);
    });

    Array<SceneDrawItem> drawList;
    drawList.Reserve(32);
    scene.ForEachSky([&](GameObject&, const SkyComponent& sk, const MeshComponent& mc, const MaterialComponent* mat,
                              const Matrix4& worldMat) {
        SceneDrawItem item{};
        SceneSubmitDetail::PopulateSkyDrawItem(item, sk, mc, mat, worldMat, params);
        drawList.PushBack(item);
    });

    scene.ForEachDrawable([&](GameObject* obj, const MeshComponent& mc, const MaterialComponent* mat,
                                 const Matrix4& worldMat) {
        if (obj != nullptr && obj->GetComponent<SkyComponent>() != nullptr) {
            return;
        }
        SceneDrawItem item{};
        item.model = worldMat;
        item.mesh = mc.GetSlot();
        if (mc.GetSlot() == SceneMeshSlot::Custom) {
            item.customMesh = mc.GetMesh();
        }
        Vector3 alb = mc.GetAlbedo();
        item.textureLayer = -1;
        if (mat != nullptr) {
            ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
            if (mat->GetBaseColorTexture()) {
                const Vector3& t = mat->GetTint();
                alb = {alb.x * t.x, alb.y * t.y, alb.z * t.z};
            }
        }
        item.albedo = alb;
        drawList.PushBack(item);
    });

    StableSortDrawItems(drawList);
    for (std::size_t di = 0; di < drawList.GetSize(); ++di) {
        params.draws.PushBack(drawList[di]);
    }

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

    PaintUiCanvases(world, params, fbW, fbH);
    helpHud.PatchSceneRenderParams(params, world);
    context.SetSceneRenderParams(params);
}

}  // namespace Spark
