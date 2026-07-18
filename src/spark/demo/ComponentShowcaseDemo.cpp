#include "spark/demo/ComponentShowcaseDemo.hpp"

#include "spark/ai/GameAiSubsystem.hpp"
#include "spark/audio/SoundClip.hpp"
#include "spark/audio/SoundFileLoader.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/demo/ShellDemoUi.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/ai/NavMeshAgentComponent.hpp"
#include "spark/ecs/components/ai/PatrolPathComponent.hpp"
#include "spark/ecs/components/audio/AmbientZoneComponent.hpp"
#include "spark/ecs/components/audio/AudioListenerComponent.hpp"
#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/HingeJoint3DComponent.hpp"
#include "spark/ecs/components/physics/3d/MeshCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SpringJoint3DComponent.hpp"
#include "spark/ecs/components/rendering/BillboardComponent.hpp"
#include "spark/ecs/components/rendering/DecalProjectorComponent.hpp"
#include "spark/ecs/components/rendering/FogVolumeComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/PostProcessVolumeComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/world/SceneSpatialPolicyComponent.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SceneSubmit.hpp"

#include <cmath>
#include <format>

namespace Spark {

namespace {

constexpr float kProbeMoveSpeed = 7.0F;

void Track(DemoRootCollection& roots, GameObject* go) noexcept {
    if (go != nullptr) {
        roots.Track(go);
    }
}

}  // namespace

void ComponentShowcaseDemo::Load(GameWorld& w, IEngineContext& context) {
    roots.Clear();
    sceneTime = 0.0F;
    dayCycleSeconds = 0.22F;
    activeStation = Station::World;
    policyRoot = nullptr;
    timeDriver = nullptr;
    probeGo = nullptr;
    probeTr = nullptr;
    patrolPathGo = nullptr;
    guardGo = nullptr;
    guardPerception = nullptr;
    dummyGo = nullptr;
    dummyHealth = nullptr;
    dummyDamageable = nullptr;
    listenerGo = nullptr;
    listenerTr = nullptr;
    hudText = nullptr;
    audioEngine = nullptr;

    unitCube = MakeShared<Mesh>(Utf8String("CompShowCube"));
    *unitCube = Mesh::CreateUnitCube();
    w.RegisterMesh(unitCube, "spark/comp_show/cube");

    unitSphere = MakeShared<Mesh>(Utf8String("CompShowSphere"));
    *unitSphere = Mesh::CreateSkySphere(0.2F, 10, 18);
    w.RegisterMesh(unitSphere, "spark/comp_show/sphere");

    groundMesh = MakeShared<Mesh>(Utf8String("CompShowGround"));
    *groundMesh = Mesh::CreateGroundPlane(kSceneGroundHalfExtent);
    w.RegisterMesh(groundMesh, "spark/comp_show/ground");

    carMesh = MakeShared<Mesh>(Utf8String("CompShowCar"));
    *carMesh = Mesh::CreateSimpleCar();
    w.RegisterMesh(carMesh, "spark/comp_show/car");

    billboardMesh = MakeShared<Mesh>(Utf8String("CompShowBillboard"));
    *billboardMesh = Mesh::CreateSkyBillboardPlane(0.7F, 0.7F);

    decalTex = MakeShared<Texture2D>(Utf8String("CompShowDecal"));
    *decalTex = Texture2D::CreateCheckerboard(
            128, 16, Vector3{0.95F, 0.35F, 0.2F}, Vector3{0.15F, 0.2F, 0.35F});
    w.RegisterTexture(decalTex, "spark/comp_show/decal");

    policyRoot = w.CreateGameObject();
    policyRoot->GetName() = Utf8String("CompShowPolicyRoot");
    policyRoot->AddComponent<SceneSpatialPolicyComponent>(ScenePartitionKind::BoundingVolumeHierarchy);
    Track(roots, policyRoot);

    BuildSharedScene(w);
    BuildWorldStation(w);
    BuildPhysicsStation(w);
    BuildAiStation(w);
    BuildAudioStation(w);
    BuildGameplayStation(w);

    listenerGo = w.CreateGameObject();
    listenerGo->GetName() = Utf8String("CompShowListener");
    listenerTr = listenerGo->AddComponent<TransformComponent>();
    listenerGo->AddComponent<AudioListenerComponent>()->SetPriority(20);
    Track(roots, listenerGo);

    probeGo = w.CreateGameObject();
    probeGo->GetName() = Utf8String("CompShowProbe");
    probeTr = probeGo->AddComponent<TransformComponent>();
    probeTr->SetTranslation({0.0F, 1.0F, 2.0F});
    probeGo->AddComponent<MeshComponent>(unitSphere, SceneMeshSlot::Custom, Vector3{0.35F, 0.9F, 1.0F});
    probeGo->AddComponent<SphereCollider3DComponent>(0.2F, Vector3::Zero);
    probeGo->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Kinematic, 1.0F);
    probeGo->AddComponent<SoundCueComponent>();
    Track(roots, probeGo);

    GameObject* hud = w.CreateGameObject();
    hud->GetName() = Utf8String("CompShowHud");
    hudText = hud->AddComponent<TextOverlayComponent>();
    hudText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
    DemoHud::Apply(*hudText);
    Track(roots, hud);

    MountUiFont(w);
    SnapCameraToStation(activeStation);
    camera.moveSpeed = 9.0F;
    camera.mouseSensitivity = 0.12F;
    context.GetInput().SetCursorCaptured(true);

    audioEngine = context.TryGetSoundEngine();
    if (audioEngine != nullptr && audioEngine->IsRunning()) {
        auto clip = TryLoadSoundClipFromBundledAsset("assets/audio/castle.wav");
        if (!clip) {
            clip = SoundClip::CreateSimpleAmbienceLoop();
        }
        audioEngine->SetBackgroundMusic(clip, 0.34F, true);
    }
}

void ComponentShowcaseDemo::Unload(GameWorld& w) {
    if (audioEngine != nullptr) {
        audioEngine->ClearBackgroundMusic();
        audioEngine = nullptr;
    }
    roots.DestroyAll(w);
    policyRoot = nullptr;
    timeDriver = nullptr;
    probeGo = nullptr;
    probeTr = nullptr;
    patrolPathGo = nullptr;
    guardGo = nullptr;
    guardPerception = nullptr;
    dummyGo = nullptr;
    dummyHealth = nullptr;
    dummyDamageable = nullptr;
    listenerGo = nullptr;
    listenerTr = nullptr;
    hudText = nullptr;
    unitCube.Reset();
    unitSphere.Reset();
    groundMesh.Reset();
    carMesh.Reset();
    billboardMesh.Reset();
    decalTex.Reset();
}

void ComponentShowcaseDemo::BuildSharedScene(GameWorld& w) {
    SharedPtr<Mesh> skyMesh = MakeShared<Mesh>(Utf8String("CompShowSkyMesh"));
    *skyMesh = Mesh::CreateSkySphere(1.0F, 16, 32);

    GameObject* sky = w.CreateGameObject();
    sky->GetName() = Utf8String("CompShowSky");
    TransformComponent* skyTr = sky->AddComponent<TransformComponent>();
    skyTr->SetUniformScale(88.0F);
    sky->AddComponent<SkyComponent>(SceneSkyMode::Dome)->SetTint({0.38F, 0.58F, 0.92F});
    sky->AddComponent<MeshComponent>(skyMesh, SceneMeshSlot::Custom, Vector3{0.45F, 0.62F, 0.95F});
    Track(roots, sky);

    GameObject* ground = w.CreateGameObject();
    ground->GetName() = Utf8String("CompShowGround");
    ground->AddComponent<TransformComponent>();
    ground->AddComponent<MeshComponent>(groundMesh, SceneMeshSlot::GroundPlane, Vector3{0.48F, 0.5F, 0.46F});
    ground->AddComponent<BoxCollider3DComponent>(
            Vector3{kSceneGroundHalfExtent, 0.2F, kSceneGroundHalfExtent}, Vector3{0.0F, 0.2F, 0.0F});
    ground->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Static, 1.0F);
    Track(roots, ground);

    GameObject* sun = w.CreateGameObject();
    sun->GetName() = Utf8String("CompShowSun");
    sun->AddComponent<TransformComponent>()->SetTranslation({10.0F, 16.0F, 8.0F});
    sun->AddComponent<PointLightComponent>(Vector3{1.0F, 0.96F, 0.88F}, 2.6F, 48.0F)->SetCastsShadow(true);
    Track(roots, sun);
}

void ComponentShowcaseDemo::BuildWorldStation(GameWorld& w) {
    GameObject* driverGo = w.CreateGameObject();
    driverGo->GetName() = Utf8String("CompShowTodDriver");
    timeDriver = driverGo->AddComponent<TimeOfDayDriverComponent>();
    timeDriver->SetDayLengthSeconds(75.0F);
    timeDriver->SetTimeOfDay(0.35F);
    timeDriver->SetLooping(true);
    Track(roots, driverGo);

    GameObject* fogGo = w.CreateGameObject();
    fogGo->GetName() = Utf8String("CompShowFog");
    fogGo->AddComponent<TransformComponent>()->SetTranslation({0.0F, 2.0F, 0.0F});
    FogVolumeComponent* fog = fogGo->AddComponent<FogVolumeComponent>();
    fog->SetHalfExtents({9.0F, 4.0F, 9.0F});
    fog->SetFogDensity(0.032F);
    fog->SetFogColor({0.58F, 0.66F, 0.78F});
    Track(roots, fogGo);

    GameObject* postGo = w.CreateGameObject();
    postGo->GetName() = Utf8String("CompShowPost");
    postGo->AddComponent<TransformComponent>()->SetTranslation({0.0F, 2.0F, 0.0F});
    PostProcessVolumeComponent* post = postGo->AddComponent<PostProcessVolumeComponent>();
    post->SetHalfExtents({7.0F, 3.5F, 7.0F});
    post->SetExposure(1.08F);
    post->SetSsaoEnabled(true);
    Track(roots, postGo);

    GameObject* marker = w.CreateGameObject();
    marker->GetName() = Utf8String("CompShowBillboard");
    marker->AddComponent<TransformComponent>()->SetTranslation({0.0F, 2.2F, 0.0F});
    marker->AddComponent<MeshComponent>(billboardMesh, SceneMeshSlot::Custom, Vector3{0.25F, 0.95F, 0.45F});
    if (MaterialComponent* bm = marker->AddComponent<MaterialComponent>()) {
        bm->SetEmissive({0.4F, 1.0F, 0.55F}, 4.0F);
    }
    BillboardComponent* bb = marker->AddComponent<BillboardComponent>();
    bb->SetMode(BillboardMode::YAxisLocked);
    Track(roots, marker);

    GameObject* decalGo = w.CreateGameObject();
    decalGo->GetName() = Utf8String("CompShowDecal");
    TransformComponent* decalTr = decalGo->AddComponent<TransformComponent>();
    decalTr->SetTranslation({0.0F, 0.03F, 0.0F});
    decalTr->SetRotation(Quaternion::FromAxisAngle(Vector3::UnitX, HalfPi));
    DecalProjectorComponent* decal = decalGo->AddComponent<DecalProjectorComponent>();
    decal->SetTexture(decalTex);
    decal->SetSize({2.8F, 2.8F, 0.4F});
    decal->SetOpacity(0.78F);
    Track(roots, decalGo);

    GameObject* sign = w.CreateGameObject();
    sign->GetName() = Utf8String("CompShowWorldSign");
    sign->AddComponent<TransformComponent>()->SetTranslation({0.0F, 0.5F, -3.5F});
    sign->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, Vector3{0.55F, 0.72F, 0.95F});
    Track(roots, sign);
}

void ComponentShowcaseDemo::BuildPhysicsStation(GameWorld& w) {
    GameObject* ramp = w.CreateGameObject();
    ramp->GetName() = Utf8String("CompShowMeshColliderRamp");
    TransformComponent* rampTr = ramp->AddComponent<TransformComponent>();
    rampTr->SetTranslation({-12.0F, 0.55F, 0.0F});
    rampTr->SetUniformScale(1.1F);
    rampTr->SetRotation(Quaternion::FromAxisAngle(Vector3::UnitY, Pi * 0.5F));
    ramp->AddComponent<MeshComponent>(carMesh, SceneMeshSlot::Custom, Vector3{0.62F, 0.58F, 0.72F});
    ramp->AddComponent<MeshCollider3DComponent>();
    ramp->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Static, 1.0F);
    ramp->AddComponent<PhysicsMaterial3DComponent>(0.55F, 0.42F, 0.35F);
    Track(roots, ramp);

    GameObject* rollingBall = w.CreateGameObject();
    rollingBall->GetName() = Utf8String("CompShowRollingBall");
    TransformComponent* ballTr = rollingBall->AddComponent<TransformComponent>();
    ballTr->SetTranslation({-12.0F, 2.4F, -2.5F});
    rollingBall->AddComponent<MeshComponent>(unitSphere, SceneMeshSlot::Custom, Vector3{0.95F, 0.4F, 0.2F});
    rollingBall->AddComponent<SphereCollider3DComponent>(0.22F, Vector3::Zero);
    Rigidbody3DComponent* ballRb = rollingBall->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Dynamic, 1.0F);
    ballRb->SetInverseMass(1.0F / 0.45F);
    ballRb->SetRestitution(0.82F);
    rollingBall->AddComponent<PhysicsMaterial3DComponent>(0.4F, 0.35F, 0.88F);
    Track(roots, rollingBall);

    constexpr float kAnchorY = 4.2F;
    GameObject* anchor = w.CreateGameObject();
    anchor->GetName() = Utf8String("CompShowSpringAnchor");
    anchor->AddComponent<TransformComponent>()->SetTranslation({-14.5F, kAnchorY, 3.0F});
    anchor->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, Vector3{0.4F, 0.42F, 0.48F});
    anchor->AddComponent<SphereCollider3DComponent>(0.12F, Vector3::Zero);
    anchor->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Static, 1.0F);
    Track(roots, anchor);

    constexpr float kRestLen = 1.8F;
    GameObject* bob = w.CreateGameObject();
    bob->GetName() = Utf8String("CompShowSpringBob");
    bob->AddComponent<TransformComponent>()->SetTranslation({-14.5F, kAnchorY - kRestLen, 3.0F});
    bob->AddComponent<MeshComponent>(unitSphere, SceneMeshSlot::Custom, Vector3{0.9F, 0.75F, 0.2F});
    bob->AddComponent<SphereCollider3DComponent>(0.16F, Vector3::Zero);
    Rigidbody3DComponent* bobRb = bob->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Dynamic, 1.0F);
    bobRb->SetInverseMass(1.0F / 0.3F);
    SpringJoint3DComponent* spring = bob->AddComponent<SpringJoint3DComponent>(anchor, kRestLen);
    spring->SetSpringStiffness(36.0F);
    spring->SetDamping(3.6F);
    Track(roots, bob);

    GameObject* hingePost = w.CreateGameObject();
    hingePost->GetName() = Utf8String("CompShowHingePost");
    hingePost->AddComponent<TransformComponent>()->SetTranslation({-10.0F, 1.2F, 3.5F});
    hingePost->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, Vector3{0.35F, 0.72F, 0.38F});
    hingePost->AddComponent<SphereCollider3DComponent>(0.1F, Vector3::Zero);
    hingePost->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Static, 1.0F);
    Track(roots, hingePost);

    GameObject* door = w.CreateGameObject();
    door->GetName() = Utf8String("CompShowHingeDoor");
    TransformComponent* doorTr = door->AddComponent<TransformComponent>();
    doorTr->SetTranslation({-10.0F, 1.2F, 4.8F});
    doorTr->SetUniformScale(0.35F);
    door->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, Vector3{0.72F, 0.55F, 0.32F});
    door->AddComponent<SphereCollider3DComponent>(0.28F, Vector3::Zero);
    Rigidbody3DComponent* doorRb = door->AddComponent<Rigidbody3DComponent>(RigidbodyBodyType3D::Dynamic, 1.0F);
    doorRb->SetInverseMass(1.0F / 2.0F);
    doorRb->SetGravityScale(0.0F);
    HingeJoint3DComponent* hinge = door->AddComponent<HingeJoint3DComponent>(hingePost);
    hinge->SetLocalAnchorA({0.0F, 0.0F, -0.5F});
    hinge->SetLocalAnchorB({0.0F, 0.0F, 0.5F});
    hinge->SetStiffness(0.7F);
    Track(roots, door);
}

void ComponentShowcaseDemo::BuildAiStation(GameWorld& w) {
    patrolPathGo = w.CreateGameObject();
    patrolPathGo->GetName() = Utf8String("CompShowPatrolPath");
    patrolPathGo->AddComponent<TransformComponent>()->SetTranslation({12.0F, 0.0F, 0.0F});
    PatrolPathComponent* path = patrolPathGo->AddComponent<PatrolPathComponent>();
    path->SetLooping(true);
    const float leg = 3.5F;
    path->GetWaypoints().PushBack(Vector3::Zero);
    path->GetWaypoints().PushBack({leg, 0.0F, 0.0F});
    path->GetWaypoints().PushBack({leg, 0.0F, leg});
    path->GetWaypoints().PushBack({0.0F, 0.0F, leg});
    Track(roots, patrolPathGo);

    guardGo = w.CreateGameObject();
    guardGo->GetName() = Utf8String("CompShowGuard");
    guardGo->AddComponent<TransformComponent>()->SetTranslation({12.0F, 0.85F, 0.0F});
    guardGo->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, Vector3{0.92F, 0.28F, 0.22F});
    NavMeshAgentComponent* nav = guardGo->AddComponent<NavMeshAgentComponent>();
    nav->SetPatrolPathObject(patrolPathGo);
    AiAgentComponent* agent = guardGo->AddComponent<AiAgentComponent>();
    agent->SetMaxSpeed(2.8F);
    agent->SetSteeringPlane(AiSteeringPlane::XzWorld);
    guardPerception = guardGo->AddComponent<PerceptionSensorComponent>();
    guardPerception->SetSightRadius(10.0F);
    guardPerception->SetSightFovDegrees(125.0F);
    Track(roots, guardGo);
}

void ComponentShowcaseDemo::BuildAudioStation(GameWorld& w) {
    GameObject* zoneGo = w.CreateGameObject();
    zoneGo->GetName() = Utf8String("CompShowAmbientZone");
    zoneGo->AddComponent<TransformComponent>()->SetTranslation({0.0F, 2.0F, 12.0F});
    AmbientZoneComponent* zone = zoneGo->AddComponent<AmbientZoneComponent>();
    zone->SetHalfExtents({5.0F, 3.0F, 5.0F});
    zone->SetVolumeScale(0.45F);
    zone->SetLowPassAmount(0.35F);
    zone->SetPriority(2);
    Track(roots, zoneGo);

    GameObject* pillar = w.CreateGameObject();
    pillar->GetName() = Utf8String("CompShowAudioPillar");
    pillar->AddComponent<TransformComponent>()->SetTranslation({0.0F, 1.0F, 12.0F});
    pillar->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, Vector3{0.45F, 0.82F, 0.95F});
    if (MaterialComponent* pm = pillar->AddComponent<MaterialComponent>()) {
        pm->SetEmissive({0.5F, 0.75F, 1.0F}, 2.2F);
    }
    Track(roots, pillar);
}

void ComponentShowcaseDemo::BuildGameplayStation(GameWorld& w) {
    dummyGo = w.CreateGameObject();
    dummyGo->GetName() = Utf8String("CompShowDummy");
    dummyGo->AddComponent<TransformComponent>()->SetTranslation({0.0F, 0.9F, -10.0F});
    dummyGo->AddComponent<MeshComponent>(unitCube, SceneMeshSlot::UnitCube, Vector3{0.55F, 0.9F, 0.35F});
    dummyHealth = dummyGo->AddComponent<HealthComponent>(5.0F);
    dummyDamageable = dummyGo->AddComponent<DamageableComponent>();
    dummyGo->AddComponent<SoundCueComponent>();
    Track(roots, dummyGo);
}

void ComponentShowcaseDemo::SnapCameraToStation(const Station station) noexcept {
    switch (station) {
        case Station::World:
            camera.position = {0.0F, 4.5F, 13.0F};
            camera.SnapLookAt({0.0F, 1.2F, 0.0F});
            break;
        case Station::Physics:
            camera.position = {-15.0F, 3.8F, 7.0F};
            camera.SnapLookAt({-12.0F, 1.2F, 0.5F});
            break;
        case Station::Ai:
            camera.position = {16.0F, 3.8F, 6.0F};
            camera.SnapLookAt({12.0F, 0.9F, 1.5F});
            break;
        case Station::Audio:
            camera.position = {0.0F, 3.5F, 7.0F};
            camera.SnapLookAt({0.0F, 1.2F, 12.0F});
            break;
        case Station::Gameplay:
            camera.position = {0.0F, 3.2F, -6.0F};
            camera.SnapLookAt({0.0F, 0.9F, -10.0F});
            break;
        default:
            break;
    }
}

void ComponentShowcaseDemo::AdvanceStation(const int delta) noexcept {
    const auto count = static_cast<int>(Station::Count);
    int idx = static_cast<int>(activeStation);
    idx = (idx + delta + count) % count;
    activeStation = static_cast<Station>(idx);
    SnapCameraToStation(activeStation);
}

const char* ComponentShowcaseDemo::StationLabel(const Station s) noexcept {
    switch (s) {
        case Station::World:
            return "World / rendering";
        case Station::Physics:
            return "Physics 3D";
        case Station::Ai:
            return "AI";
        case Station::Audio:
            return "Audio";
        case Station::Gameplay:
            return "Gameplay";
        default:
            return "Station";
    }
}

const char* ComponentShowcaseDemo::StationComponents(const Station s) noexcept {
    switch (s) {
        case Station::World:
            return "TimeOfDayDriver, FogVolume, PostProcessVolume, Billboard, DecalProjector, SceneSpatialPolicy";
        case Station::Physics:
            return "MeshCollider3D, SpringJoint3D, HingeJoint3D, PhysicsMaterial3D";
        case Station::Ai:
            return "PatrolPath, NavMeshAgent, AiAgent, PerceptionSensor (+ SimulateGameAi)";
        case Station::Audio:
            return "AmbientZone, AudioListener, SoundCue (move probe: E)";
        case Station::Gameplay:
            return "Health, Damageable, SoundCue (K = hurt dummy)";
        default:
            return "";
    }
}

void ComponentShowcaseDemo::Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world) {
    sceneTime = timing.totalTimeSeconds;
    const float dt = timing.deltaTimeSeconds;
    IInput& in = context.GetInput();

    if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        in.SetCursorCaptured(!in.IsCursorCaptured());
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_TAB)) {
        AdvanceStation(1);
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_LEFT_BRACKET)) {
        AdvanceStation(-1);
    }
    if (in.IsCursorCaptured() && timing.frameIndex > 0U) {
        camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
    }
    camera.ProcessMovement(in, dt);

    if (timeDriver != nullptr) {
        dayCycleSeconds += dt * 0.08F;
        timeDriver->SetTimeOfDay(std::fmod(dayCycleSeconds, 1.0F));
    }

    if (probeTr != nullptr && probeGo != nullptr) {
        Vector3 p = probeTr->GetLocalTransform().translation;
        float mx = 0.0F;
        float mz = 0.0F;
        if (in.IsKeyDown(GLFW_KEY_E)) {
            const Vector3 f = camera.Forward();
            Vector3 flatF{f.x, 0.0F, f.z};
            if (flatF.LengthSquared() > 1.0e-8F) {
                flatF = flatF.Normalized();
                p += flatF * kProbeMoveSpeed * dt;
            }
        }
        if (in.IsKeyDown(GLFW_KEY_LEFT) || in.IsKeyDown(GLFW_KEY_A)) {
            mx -= 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_RIGHT) || in.IsKeyDown(GLFW_KEY_D)) {
            mx += 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_UP)) {
            mz -= 1.0F;
        }
        if (in.IsKeyDown(GLFW_KEY_DOWN)) {
            mz += 1.0F;
        }
        p.x += mx * kProbeMoveSpeed * dt;
        p.z += mz * kProbeMoveSpeed * dt;
        probeTr->SetTranslation(p);
    }

    if (in.IsKeyPressedThisFrame(GLFW_KEY_K) && dummyDamageable != nullptr && dummyGo != nullptr) {
        dummyDamageable->ApplyDamage(1.0F, probeGo);
        DemoAudio::QueueCue(*dummyGo, DemoSfx::ClipPhysicsThrow(), 0.55F);
    }
    if (in.IsKeyPressedThisFrame(GLFW_KEY_R) && dummyHealth != nullptr) {
        dummyHealth->ResetToFull();
    }

    PhysicsWorld3DSettings phys{};
    phys.gravityY = -9.81F;
    phys.resolveIterations = 10;
    phys.substeps = 2;
    phys.jointIterations = 8;
    SimulatePhysics3D(world, timing, phys);
    SimulateGameAi(world, timing, context);

    if (listenerTr != nullptr) {
        listenerTr->SetTranslation(camera.position);
        const Vector3 fwd = camera.Forward().Normalized();
        if (fwd.LengthSquared() > 1.0e-8F) {
            const float yaw = std::atan2(fwd.x, -fwd.z);
            listenerTr->SetRotation(Quaternion::FromAxisAngle(Vector3::UnitY, yaw));
        }
    }

    if (hudText != nullptr) {
        const float fps = fpsHud.Update(dt, timing.frameIndex);
        bool guardAlert = false;
        if (guardPerception != nullptr && probeGo != nullptr) {
            const Array<GameObject*>& detected = guardPerception->GetDetectedObjects();
            for (std::size_t i = 0; i < detected.GetSize(); ++i) {
                if (detected[i] == probeGo) {
                    guardAlert = true;
                    break;
                }
            }
        }
        float hp = 0.0F;
        float hpMax = 0.0F;
        if (dummyHealth != nullptr) {
            hp = dummyHealth->GetCurrent();
            hpMax = dummyHealth->GetMaximum();
        }
        hudText->SetText(Utf8String(
                std::format(
                        "Component showcase — {} — {}\n"
                        "TAB/[] stations · WASD fly · arrows/E move probe · K hurt dummy · R reset HP · guard {} · "
                        "dummy {:.0f}/{:.0f} HP · {:.0f} FPS · ESC menu\n"
                        "2D extras (PhysicsMaterial2D, HingeJoint2D, DistanceJoint2D): Platformer2D & Maze2D demos",
                        StationLabel(activeStation),
                        StationComponents(activeStation),
                        guardAlert ? "ALERT" : "patrol",
                        static_cast<double>(hp),
                        static_cast<double>(hpMax),
                        static_cast<double>(fps))
                        .c_str()));
    }
}

void ComponentShowcaseDemo::Render(Scene& scene, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    scene.ApplySpatialPolicyFromFirstMatchingObject();

    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.12F, 320.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    Vector3 forward = camera.Forward();
    Vector3 pr = Vector3::Cross(Vector3{0.0F, 1.0F, 0.0F}, forward).Normalized();
    if (pr.LengthSquared() < 1.0e-8F) {
        pr = Vector3::Cross(Vector3{1.0F, 0.0F, 0.0F}, forward).Normalized();
    }
    const Vector3 pu = Vector3::Cross(forward, pr).Normalized();

    SubmitStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Vector3{0.32F, 0.86F, 0.38F}.Normalized(),
            Vector3{1.0F, 0.96F, 0.9F},
            0.9F,
            Vector3{0.09F, 0.11F, 0.14F},
            true,
            pr,
            pu,
            sceneTime);
}

}  // namespace Spark
