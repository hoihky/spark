#include "spark/demo/Maze3DDemo.hpp"
#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/demo/DemoFoundation.hpp"
#include "spark/audio/SoundFileLoader.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/ai/GameAiSubsystem.hpp"

namespace Spark {
namespace {

struct Maze3DCell {
    int i = 0;
    int j = 0;
};

void ShuffleMazeCells3(Array<Maze3DCell>& cells, unsigned seed) noexcept {
    for (std::size_t n = cells.GetSize(); n > 1U; --n) {
        seed = seed * 1664525U + 1013904223U;
        const std::size_t k = static_cast<std::size_t>(seed % static_cast<unsigned>(n));
        Swap(cells[k], cells[n - 1U]);
    }
}

void GenerateMazeOddGrid3(int w, int h, Array<std::uint8_t>& outCells) {
    outCells.Clear();
    const std::size_t n = static_cast<std::size_t>(w * h);
    outCells.Resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        outCells[i] = 1;
    }
    static constexpr int kDirs[4][2] = {
            {0, -2},
            {0, 2},
            {-2, 0},
            {2, 0},
    };

    Array<Maze3DCell> stack;
    stack.PushBack(Maze3DCell{1, 1});
    outCells[static_cast<std::size_t>(1 * w + 1)] = 0;

    while (!stack.IsEmpty()) {
        const Maze3DCell cur = stack[stack.GetSize() - 1];
        const int x = cur.i;
        const int y = cur.j;
        int ord[4] = {0, 1, 2, 3};
        unsigned seed = static_cast<unsigned>(x * 1103515245 + y * 12345 + 7);
        for (int a = 3; a > 0; --a) {
            const int b = static_cast<int>((seed >> (a * 3)) % static_cast<unsigned>(a + 1));
            const int tmp = ord[a];
            ord[a] = ord[b];
            ord[b] = tmp;
        }

        bool advanced = false;
        for (int k = 0; k < 4; ++k) {
            const int* d = kDirs[ord[k]];
            const int nx = x + d[0];
            const int ny = y + d[1];
            if (nx < 1 || ny < 1 || nx >= w - 1 || ny >= h - 1) {
                continue;
            }
            if (outCells[static_cast<std::size_t>(ny * w + nx)] == 0) {
                continue;
            }
            const int mx = x + d[0] / 2;
            const int my = y + d[1] / 2;
            outCells[static_cast<std::size_t>(my * w + mx)] = 0;
            outCells[static_cast<std::size_t>(ny * w + nx)] = 0;
            stack.PushBack(Maze3DCell{nx, ny});
            advanced = true;
            break;
        }
        if (!advanced) {
            stack.PopBack();
        }
    }
}

void RotateAabbByQuaternion(
        const Spark::Vector3& bmin,
        const Spark::Vector3& bmax,
        const Spark::Quaternion& q,
        Spark::Vector3& outMin,
        Spark::Vector3& outMax) noexcept {
    static constexpr int kIx[8][3] = {
            {0, 0, 0},
            {1, 0, 0},
            {0, 1, 0},
            {1, 1, 0},
            {0, 0, 1},
            {1, 0, 1},
            {0, 1, 1},
            {1, 1, 1},
    };
    bool first = true;
    for (const auto& id : kIx) {
        const Spark::Vector3 c{
                id[0] ? bmax.x : bmin.x,
                id[1] ? bmax.y : bmin.y,
                id[2] ? bmax.z : bmin.z};
        const Spark::Vector3 r = q.RotateVector(c);
        if (first) {
            outMin = r;
            outMax = r;
            first = false;
        } else {
            outMin.x = std::min(outMin.x, r.x);
            outMin.y = std::min(outMin.y, r.y);
            outMin.z = std::min(outMin.z, r.z);
            outMax.x = std::max(outMax.x, r.x);
            outMax.y = std::max(outMax.y, r.y);
            outMax.z = std::max(outMax.z, r.z);
        }
    }
}

bool FindSpawnCorridorIJ(const Array<std::uint8_t>& cells, int w, int h, int& outI, int& outJ) noexcept {
    if (w > 3 && h > 3 && cells[static_cast<std::size_t>(1 * w + 1)] == 0) {
        outI = 1;
        outJ = 1;
        return true;
    }
    for (int j = 1; j < h - 1; ++j) {
        for (int i = 1; i < w - 1; ++i) {
            if (cells[static_cast<std::size_t>(j * w + i)] == 0) {
                outI = i;
                outJ = j;
                return true;
            }
        }
    }
    return false;
}

}  // namespace

void Maze3DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        gemObjects.Clear();
        wallCount = 0;
        gemsCollected = 0;
        gemsTotal = 0;
        playerAnimator = nullptr;
        playerCharAnimFsm = nullptr;
        patrolPathGo = nullptr;
        guardGo = nullptr;
        guardPerception = nullptr;
        useHumanAvatar = false;
        humanModelYawOffset = 0.0F;
        humanModelBindFix = Spark::Quaternion::Identity;
        characterAvatarHudName = Spark::Utf8String{};
        mazeSkyObject = nullptr;
        mazeSkyTransform = nullptr;
        mazeSkyMeshComp = nullptr;
        mazeSkyComp = nullptr;
        mazeSkyMat = nullptr;

        Array<std::uint8_t> cells;
        GenerateMazeOddGrid3(kMazeW, kMazeH, cells);

        int spawnI = 1;
        int spawnJ = 1;
        if (!FindSpawnCorridorIJ(cells, kMazeW, kMazeH, spawnI, spawnJ)) {
            spawnI = 1;
            spawnJ = 1;
        }

        skyBoxMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("Maze3DSkySphere"));
        *skyBoxMesh = Spark::Mesh::CreateSkySphere(1.0F, 20, 40);
        skyEquirectTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("Maze3DSkyEquirect"));
        mazeSkyHasEquirect = false;
        Spark::Texture2D skyDecoded;
        if (Spark::Texture2D::TryLoadFromFile(SPARK_SKY_TEXTURE_PATH, skyDecoded)) {
            *skyEquirectTex = Spark::MoveTemp(skyDecoded);
            mazeSkyHasEquirect = true;
            w.RegisterTexture(skyEquirectTex, "spark/maze3d/sky_equirect");
        } else {
            Spark::Utf8String altSky(SPARK_ASSETS_DIR);
            altSky.AppendUtf8("/textures/sky/equirect_sky_1k.hdr");
            if (Spark::Texture2D::TryLoadFromFile(altSky.CStr(), skyDecoded)) {
                *skyEquirectTex = Spark::MoveTemp(skyDecoded);
                mazeSkyHasEquirect = true;
                w.RegisterTexture(skyEquirectTex, "spark/maze3d/sky_equirect");
            }
        }

        mazeSkyObject = w.CreateGameObject();
        mazeSkyObject->GetName() = Spark::Utf8String("Maze3DSky");
        mazeSkyTransform = mazeSkyObject->AddComponent<Spark::TransformComponent>();
        mazeSkyMeshComp = mazeSkyObject->AddComponent<Spark::MeshComponent>(skyBoxMesh, Spark::Vector3::One);
        mazeSkyComp = mazeSkyObject->AddComponent<Spark::SkyComponent>(Spark::SceneSkyMode::Box);
        mazeSkyMat = mazeSkyObject->AddComponent<Spark::MaterialComponent>();
        roots.PushBack(mazeSkyObject);
        ApplyMazeSkyVisuals();

        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("Maze3DUnitCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        w.RegisterMesh(unitCubeAsset, "spark/maze3d/unit_cube");

        groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("Maze3DGround"));
        *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent);
        w.RegisterMesh(groundAsset, "spark/maze3d/ground");

        wallBrickTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("Maze3DWallBrick"));
        if (!DemoAssets::TryLoadWallBrickStoneTexture(*wallBrickTex)) {
            *wallBrickTex = Spark::Texture2D::CreateBrickPattern(256, 256);
            w.RegisterTexture(wallBrickTex, "spark/maze3d/wall_brick_fallback");
        } else {
            w.RegisterTexture(wallBrickTex, "spark/maze3d/wall_bricks");
        }

        Spark::GameObject* ground = w.CreateGameObject();
        ground->GetName() = Spark::Utf8String("Maze3DGround");
        Spark::TransformComponent* groundTr = ground->AddComponent<Spark::TransformComponent>();
        {
            const float mazeNeedHalf =
                    0.5F * static_cast<float>(std::max(kMazeW, kMazeH)) * kCellWorld + 10.0F;
            const float gScale = std::max(1.0F, mazeNeedHalf / Spark::kSceneGroundHalfExtent);
            groundTr->SetUniformScale(gScale);
        }
        ground->AddComponent<Spark::MeshComponent>(
                groundAsset, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3{0.52F, 0.55F, 0.58F});
        roots.PushBack(ground);

        const float originX = -0.5F * static_cast<float>(kMazeW) * kCellWorld;
        const float originZ = -0.5F * static_cast<float>(kMazeH) * kCellWorld;
        const float wallScale = 0.5F * kCellWorld;
        const Spark::Vector3 wallHalf{1.0F, 1.0F, 1.0F};

        for (int j = 0; j < kMazeH; ++j) {
            for (int i = 0; i < kMazeW; ++i) {
                if (cells[static_cast<std::size_t>(j * kMazeW + i)] == 0) {
                    continue;
                }
                Spark::GameObject* g = w.CreateGameObject();
                g->GetName() = Spark::Utf8String("Maze3DWall");
                Spark::TransformComponent* tr = g->AddComponent<Spark::TransformComponent>();
                const float cx = originX + (static_cast<float>(i) + 0.5F) * kCellWorld;
                const float cz = originZ + (static_cast<float>(j) + 0.5F) * kCellWorld;
                tr->SetTranslation({cx, wallScale, cz});
                tr->SetScale({wallScale, wallScale, wallScale});
                g->AddComponent<Spark::MeshComponent>(
                        unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{1.0F, 1.0F, 1.0F});
                if (Spark::MaterialComponent* m = g->AddComponent<Spark::MaterialComponent>(
                            wallBrickTex, Spark::Vector3::One)) {
                    m->SetMetallic(0.0F);
                    m->SetRoughness(0.88F);
                }
                g->AddComponent<Spark::BoxCollider3DComponent>(wallHalf);
                roots.PushBack(g);
                ++wallCount;
            }
        }

        const float px = originX + (static_cast<float>(spawnI) + 0.5F) * kCellWorld;
        const float pz = originZ + (static_cast<float>(spawnJ) + 0.5F) * kCellWorld;

        playerGo = w.CreateGameObject();
        playerGo->GetName() = Spark::Utf8String("Maze3DPlayer");
        playerTr = playerGo->AddComponent<Spark::TransformComponent>();
        playerTr->SetTranslation({px, 0.0F, pz});
        constexpr float kTorsoSphereRadius = 0.34F;
        constexpr float kTorsoSphereCenterY = 0.92F;
        playerGo->AddComponent<Spark::SphereCollider3DComponent>(
                kTorsoSphereRadius, Spark::Vector3{0.0F, kTorsoSphereCenterY, 0.0F});
        playerRb = playerGo->AddComponent<Spark::Rigidbody3DComponent>(Spark::RigidbodyBodyType3D::Dynamic, 1.0F);
        playerRb->SetVelocity(Spark::Vector3::Zero);

        Spark::Utf8String cesiumPath(SPARK_ASSETS_DIR);
        cesiumPath.AppendUtf8("/models/CesiumMan.glb");
        Spark::SkinnedGltfAsset humanAsset = w.LoadSkinnedGltf(cesiumPath.CStr());
        const bool loadedCesium = humanAsset.mesh && humanAsset.skeleton;
        if (!loadedCesium) {
            Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
            foxPath.AppendUtf8("/models/Fox.glb");
            humanAsset = w.LoadSkinnedGltf(foxPath.CStr());
        }
        if (humanAsset.mesh && humanAsset.skeleton) {
            useHumanAvatar = true;
            if (humanAsset.baseColorTexture) {
                w.RegisterTexture(humanAsset.baseColorTexture, "spark/maze3d/human_basecolor");
            }
            humanModelBindFix = humanAsset.bindUpAlignment.Normalized();
            humanModelYawOffset = humanAsset.bindFacingYawOffset;
            if (!loadedCesium) {
                humanModelYawOffset = -Spark::HalfPi;
            }

            Spark::Vector3 bmin{};
            Spark::Vector3 bmax{};
            bool haveBounds = false;
            {
                const Spark::Array<Spark::SkinnedMesh::Vertex>& sv = humanAsset.mesh->GetVertices();
                if (!sv.IsEmpty()) {
                    haveBounds = true;
                    bmin = sv[0].position;
                    bmax = sv[0].position;
                    for (std::size_t vi = 1; vi < sv.GetSize(); ++vi) {
                        const Spark::Vector3& p = sv[vi].position;
                        bmin.x = std::min(bmin.x, p.x);
                        bmin.y = std::min(bmin.y, p.y);
                        bmin.z = std::min(bmin.z, p.z);
                        bmax.x = std::max(bmax.x, p.x);
                        bmax.y = std::max(bmax.y, p.y);
                        bmax.z = std::max(bmax.z, p.z);
                    }
                }
            }
            Spark::Vector3 rbmin = bmin;
            Spark::Vector3 rbmax = bmax;
            if (haveBounds) {
                RotateAabbByQuaternion(bmin, bmax, humanModelBindFix, rbmin, rbmax);
            }
            constexpr float kTargetHeightM = 1.75F;
            float sc = 0.04F;
            if (haveBounds) {
                const float h = std::max(1.0e-4F, rbmax.y - rbmin.y);
                sc = kTargetHeightM / h;
            }
            Spark::GameObject* vis = w.CreateGameObject();
            vis->GetName() = Spark::Utf8String("Maze3DPlayerVisual");
            vis->SetParent(playerGo);
            Spark::TransformComponent* vtr = vis->AddComponent<Spark::TransformComponent>();
            vtr->SetUniformScale(sc);
            if (haveBounds) {
                vtr->SetTranslation({0.0F, -rbmin.y * sc, 0.0F});
            }
            vis->AddComponent<Spark::SkinnedMeshComponent>(humanAsset.mesh);
            playerCharAnimFsm = vis->AddComponent<Spark::Character3DAnimFsmComponent>();
            playerAnimator = vis->AddComponent<Spark::AnimatorComponent>(
                    humanAsset.skeleton, humanAsset.walkClipIndex, 1.0F);
            playerCharAnimFsm->ConfigureLocomotionFromSkeleton(*humanAsset.skeleton, humanAsset.walkClipIndex);
            playerCharAnimFsm->SetWalkSpeedThreshold(0.35F);
            playerCharAnimFsm->SetRunSpeedThreshold(2.5F);
            if (humanAsset.baseColorTexture) {
                if (Spark::MaterialComponent* fm = vis->AddComponent<Spark::MaterialComponent>(
                            humanAsset.baseColorTexture, Spark::Vector3::One)) {
                    fm->SetMetallic(0.0F);
                    fm->SetRoughness(0.55F);
                }
            } else {
                vis->AddComponent<Spark::MaterialComponent>();
            }
            characterAvatarHudName = loadedCesium ? Spark::Utf8String("CesiumMan")
                                                  : Spark::Utf8String("Fox (fallback)");
        } else {
            useHumanAvatar = false;
            characterAvatarHudName = Spark::Utf8String("Cube (glTF missing)");
            Spark::GameObject* vis = w.CreateGameObject();
            vis->GetName() = Spark::Utf8String("Maze3DPlayerVisual");
            vis->SetParent(playerGo);
            Spark::TransformComponent* vtr = vis->AddComponent<Spark::TransformComponent>();
            vtr->SetTranslation({0.0F, 0.21F, 0.0F});
            vtr->SetUniformScale(0.42F);
            vis->AddComponent<Spark::MeshComponent>(
                    unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{1.0F, 0.92F, 0.08F});
        }

        roots.PushBack(playerGo);

        {
            const float pathX = originX + (static_cast<float>(kMazeW / 2) + 0.5F) * kCellWorld;
            const float pathZ = originZ + (static_cast<float>(kMazeH / 2) + 0.5F) * kCellWorld;
            patrolPathGo = w.CreateGameObject();
            patrolPathGo->GetName() = Spark::Utf8String("Maze3DPatrolPath");
            Spark::TransformComponent* pathTr = patrolPathGo->AddComponent<Spark::TransformComponent>();
            pathTr->SetTranslation({pathX, 0.0F, pathZ});
            Spark::PatrolPathComponent* patrol = patrolPathGo->AddComponent<Spark::PatrolPathComponent>();
            patrol->SetLooping(true);
            const float leg = kCellWorld * 2.0F;
            patrol->GetWaypoints().PushBack(Spark::Vector3::Zero);
            patrol->GetWaypoints().PushBack({leg, 0.0F, 0.0F});
            patrol->GetWaypoints().PushBack({leg, 0.0F, leg});
            patrol->GetWaypoints().PushBack({0.0F, 0.0F, leg});
            roots.PushBack(patrolPathGo);

            guardGo = w.CreateGameObject();
            guardGo->GetName() = Spark::Utf8String("Maze3DGuard");
            Spark::TransformComponent* guardTr = guardGo->AddComponent<Spark::TransformComponent>();
            guardTr->SetTranslation({pathX, 0.85F, pathZ});
            guardTr->SetUniformScale(0.55F);
            guardGo->AddComponent<Spark::MeshComponent>(
                    unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.92F, 0.22F, 0.18F});
            if (Spark::MaterialComponent* gm = guardGo->AddComponent<Spark::MaterialComponent>()) {
                gm->SetEmissive({0.95F, 0.28F, 0.12F}, 2.4F);
                gm->SetRoughness(0.4F);
            }
            Spark::NavMeshAgentComponent* nav = guardGo->AddComponent<Spark::NavMeshAgentComponent>();
            nav->SetPatrolPathObject(patrolPathGo);
            Spark::AiAgentComponent* agent = guardGo->AddComponent<Spark::AiAgentComponent>();
            agent->SetMaxSpeed(3.2F);
            agent->SetSteeringPlane(Spark::AiSteeringPlane::XzWorld);
            guardPerception = guardGo->AddComponent<Spark::PerceptionSensorComponent>();
            guardPerception->SetSightRadius(9.5F);
            guardPerception->SetSightFovDegrees(130.0F);
            guardPerception->SetHearingRadius(6.0F);
            roots.PushBack(guardGo);
        }

        Array<Maze3DCell> floorCells;
        for (int j = 0; j < kMazeH; ++j) {
            for (int i = 0; i < kMazeW; ++i) {
                if (cells[static_cast<std::size_t>(j * kMazeW + i)] != 0) {
                    continue;
                }
                if (i == spawnI && j == spawnJ) {
                    continue;
                }
                if ((i == 1 && j == 1) || (i == 1 && j == 2) || (i == 2 && j == 1)) {
                    continue;
                }
                floorCells.PushBack(Maze3DCell{i, j});
            }
        }
        ShuffleMazeCells3(floorCells, static_cast<unsigned>(kMazeW * 49999 + kMazeH * 131U + 29U));
        constexpr int kMaxGems = 14;
        gemsTotal = static_cast<int>(floorCells.GetSize());
        if (gemsTotal > kMaxGems) {
            gemsTotal = kMaxGems;
        }
        for (int gi = 0; gi < gemsTotal; ++gi) {
            const Maze3DCell c = floorCells[static_cast<std::size_t>(gi)];
            Spark::GameObject* gem = w.CreateGameObject();
            gem->GetName() = Spark::Utf8String("Maze3DGem");
            Spark::TransformComponent* gtr = gem->AddComponent<Spark::TransformComponent>();
            const float gx = originX + (static_cast<float>(c.i) + 0.5F) * kCellWorld;
            const float gz = originZ + (static_cast<float>(c.j) + 0.5F) * kCellWorld;
            gtr->SetTranslation({gx, 0.28F, gz});
            gtr->SetUniformScale(0.22F);
            const float hue = static_cast<float>(gi) * 0.37F;
            const Spark::Vector3 rgb{
                    0.35F + 0.45F * std::fabs(std::sin(hue)),
                    0.55F + 0.35F * std::fabs(std::sin(hue + 2.1F)),
                    0.85F + 0.15F * std::fabs(std::sin(hue + 4.2F))};
            gem->AddComponent<Spark::MeshComponent>(
                    unitCubeAsset, Spark::SceneMeshSlot::UnitCube, rgb);
            if (Spark::MaterialComponent* gm = gem->AddComponent<Spark::MaterialComponent>()) {
                gm->SetEmissive({rgb.x * 1.4F, rgb.y * 1.35F, rgb.z * 1.3F}, 3.2F);
                gm->SetRoughness(0.35F);
            }
            if (Spark::ParticleEmitterComponent* pe = gem->AddComponent<Spark::ParticleEmitterComponent>()) {
                pe->SetMaxParticles(200);
                pe->SetEmissionRate(32.0F);
                pe->SetLifetime(0.4F, 1.05F);
                pe->SetStartEndSize(0.11F, 0.018F);
                const Spark::Vector4 c0{
                        std::min(1.0F, rgb.x * 1.25F),
                        std::min(1.0F, rgb.y * 1.22F),
                        std::min(1.0F, rgb.z * 1.18F),
                        1.0F};
                const Spark::Vector4 c1{rgb.x * 0.55F, rgb.y * 0.5F, rgb.z * 0.85F, 0.0F};
                pe->SetStartEndColor(c0, c1);
                pe->SetGravity({0.0F, 0.55F, 0.0F});
                pe->SetEmissionDirection({0.0F, 1.0F, 0.0F});
                pe->SetSpreadAngleRadians(1.25F);
                pe->SetSpeedRange(0.25F, 1.05F);
            }
            gemObjects.PushBack(gem);
        }

        Spark::GameObject* lightA = w.CreateGameObject();
        lightA->GetName() = Spark::Utf8String("Maze3DLightA");
        Spark::TransformComponent* lta = lightA->AddComponent<Spark::TransformComponent>();
        lta->SetTranslation({originX + 8.0F, 9.0F, originZ + 6.0F});
        lightA->AddComponent<Spark::PointLightComponent>(Spark::Vector3{0.4F, 0.75F, 1.0F}, 5.0F, 28.0F)
                ->SetCastsShadow(true);
        roots.PushBack(lightA);

        Spark::GameObject* lightB = w.CreateGameObject();
        lightB->GetName() = Spark::Utf8String("Maze3DLightB");
        Spark::TransformComponent* ltb = lightB->AddComponent<Spark::TransformComponent>();
        ltb->SetTranslation({originX + 22.0F, 6.0F, originZ + 18.0F});
        lightB->AddComponent<Spark::PointLightComponent>(Spark::Vector3{1.0F, 0.45F, 0.28F}, 4.0F, 22.0F)
                ->SetCastsShadow(true);
        roots.PushBack(lightB);

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("Maze3DFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*fpsText);
        fpsText->SetText(Spark::Utf8String(
                "3D maze — FP WASD — guard patrols (NavMeshAgent+AiAgent) — F1 mouse — ESC menu"));
        roots.PushBack(fpsHudObject);

        rig = {};
        rig.mode = Spark::CharacterCameraMode::FirstPerson;
        rig.groundY = 0.0F;
        rig.characterPosition = {px, 0.0F, pz};
        rig.moveSpeed = kCellWorld * (14.0F / 2.25F);
        rig.runSpeed = rig.moveSpeed * 1.65F;
        rig.mouseSensitivity = 0.12F;
        rig.firstPersonEyeHeight = 1.62F;
        rig.firstPersonForwardNudge = 0.22F;
        rig.cameraPitch = 0.0F;
        rig.cameraYaw = std::atan2(0.01F, -1.0F);
        rig.characterVisualYaw = rig.cameraYaw;
        rig.characterFacingYawOffset = humanModelYawOffset;
        rig.characterRootBindOrientation = humanModelBindFix;

        PhysicsWorld3DSettings& phys = physics.GetWorld3D().GetSettings();
        phys.gravityY = 0.0F;
        phys.maxFallSpeed = 500.0F;

        context.GetInput().SetCursorCaptured(true);

        if (audioEngine != nullptr) {
            audioEngine->ClearBackgroundMusic();
            audioEngine = nullptr;
        }
        audioEngine = context.TryGetSoundEngine();
        if (audioEngine != nullptr && audioEngine->IsRunning()) {
            if (Spark::SharedPtr<Spark::SoundClip> bgm =
                        TryLoadSoundClipFromBundledAsset("assets/audio/Medieval_2_6_loop.wav")) {
                audioEngine->SetBackgroundMusic(bgm, 0.28F, true);
            }
        }
    }

void Maze3DDemo::Unload(Spark::GameWorld& w)
{
        if (audioEngine != nullptr) {
            audioEngine->ClearBackgroundMusic();
            audioEngine = nullptr;
        }
        for (std::size_t i = 0; i < gemObjects.GetSize(); ++i) {
            if (gemObjects[i] != nullptr) {
                w.DestroyGameObject(gemObjects[i]);
            }
        }
        gemObjects.Clear();
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        unitCubeAsset.Reset();
        groundAsset.Reset();
        wallBrickTex.Reset();
        skyBoxMesh.Reset();
        skyEquirectTex.Reset();
        mazeSkyObject = nullptr;
        mazeSkyTransform = nullptr;
        mazeSkyMeshComp = nullptr;
        mazeSkyComp = nullptr;
        mazeSkyMat = nullptr;
        playerGo = nullptr;
        playerTr = nullptr;
        playerRb = nullptr;
        playerAnimator = nullptr;
        playerCharAnimFsm = nullptr;
        patrolPathGo = nullptr;
        guardGo = nullptr;
        guardPerception = nullptr;
        useHumanAvatar = false;
        fpsHudObject = nullptr;
        fpsText = nullptr;
    }

void Maze3DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world)
{
        Spark::IInput& in = context.GetInput();
        const float dt = timing.deltaTimeSeconds;

        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (in.IsCursorCaptured() && timing.frameIndex > 0) {
            rig.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
        }

        UpdateMazeSkyTransform();

        if (playerRb != nullptr && playerTr != nullptr) {
            const Spark::Vector3 pFeet = playerTr->GetLocalTransform().translation;
            rig.characterPosition.x = pFeet.x;
            rig.characterPosition.y = rig.groundY;
            rig.characterPosition.z = pFeet.z;

            const float sy = std::sin(rig.cameraYaw);
            const float cy = std::cos(rig.cameraYaw);
            Spark::Vector3 flatF{sy, 0.0F, -cy};
            Spark::Vector3 flatR = Spark::Vector3::Cross(flatF, Spark::Vector3::UnitY);
            if (flatR.LengthSquared() < Spark::Epsilon) {
                flatR = Spark::Vector3::UnitX;
            } else {
                flatR = flatR.Normalized();
            }

            Spark::Vector3 moveAccum{};
            if (in.IsKeyDown(GLFW_KEY_W) || in.IsKeyDown(GLFW_KEY_UP)) {
                moveAccum += flatF;
            }
            if (in.IsKeyDown(GLFW_KEY_S) || in.IsKeyDown(GLFW_KEY_DOWN)) {
                moveAccum -= flatF;
            }
            if (in.IsKeyDown(GLFW_KEY_D) || in.IsKeyDown(GLFW_KEY_RIGHT)) {
                moveAccum += flatR;
            }
            if (in.IsKeyDown(GLFW_KEY_A) || in.IsKeyDown(GLFW_KEY_LEFT)) {
                moveAccum -= flatR;
            }

            const float moveLen2 = moveAccum.LengthSquared();
            const bool moving = moveLen2 > Spark::Epsilon;
            const bool sprint = moving
                    && (in.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || in.IsKeyDown(GLFW_KEY_RIGHT_SHIFT));
            if (moving) {
                const float speed = sprint ? rig.runSpeed : rig.moveSpeed;
                const Spark::Vector3 wish = moveAccum.Normalized() * speed;
                playerRb->SetVelocity({wish.x, 0.0F, wish.z});
                rig.characterVisualYaw = std::atan2(wish.x, -wish.z);
            } else {
                playerRb->SetVelocity(Spark::Vector3::Zero);
            }

            if (playerCharAnimFsm != nullptr) {
                playerCharAnimFsm->SetLocomotionInput(moving, sprint);
            }

            {
                const Spark::Quaternion qYaw = Spark::Quaternion::FromAxisAngle(
                        Spark::Vector3::UnitY, rig.characterVisualYaw + humanModelYawOffset);
                playerTr->SetRotation(
                        useHumanAvatar ? (qYaw * humanModelBindFix).Normalized() : qYaw.Normalized());
            }

            physics.Simulate3D(world, timing);
            Spark::SimulateGameAi(world, timing, context);

            const Spark::Vector3 p = playerTr->GetLocalTransform().translation;
            rig.characterPosition.x = p.x;
            rig.characterPosition.y = rig.groundY;
            rig.characterPosition.z = p.z;

            constexpr float kCollectRadius = 0.65F * (kCellWorld / 2.25F);
            const float cr2 = kCollectRadius * kCollectRadius;
            for (std::size_t gi = 0; gi < gemObjects.GetSize();) {
                Spark::GameObject* gem = gemObjects[gi];
                if (gem == nullptr) {
                    gemObjects.RemoveAt(gi);
                    continue;
                }
                const Spark::TransformComponent* gtr = gem->GetComponent<Spark::TransformComponent>();
                if (gtr == nullptr) {
                    world.DestroyGameObject(gem);
                    gemObjects.RemoveAt(gi);
                    continue;
                }
                const Spark::Vector3 gpos = gtr->GetLocalTransform().translation;
                const float dx = gpos.x - p.x;
                const float dy = gpos.y - p.y;
                const float dz = gpos.z - p.z;
                if (dx * dx + dy * dy + dz * dz <= cr2) {
                    world.DestroyGameObject(gem);
                    gemObjects.RemoveAt(gi);
                    ++gemsCollected;
                    DemoAudio::QueueCue(*playerGo, DemoSfx::ClipGemCollect(), 0.95F);
                    continue;
                }
                ++gi;
            }
        }

        if (fpsText != nullptr) {
            const float tdt = timing.deltaTimeSeconds;
            const float instant = (tdt > 1.0e-6F) ? (1.0F / tdt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            bool guardSeesPlayer = false;
            if (guardPerception != nullptr && playerGo != nullptr) {
                const Spark::Array<Spark::GameObject*>& detected = guardPerception->GetDetectedObjects();
                for (std::size_t di = 0; di < detected.GetSize(); ++di) {
                    if (detected[di] == playerGo) {
                        guardSeesPlayer = true;
                        break;
                    }
                }
            }
            const std::string hud = std::format(
                    "3D maze {}×{} — {} — {} walls — gems {}/{} — guard {} — {:.0f} FPS — FP WASD — F1 — ESC",
                    kMazeW,
                    kMazeH,
                    characterAvatarHudName.CStr(),
                    wallCount,
                    gemsCollected,
                    gemsTotal,
                    guardSeesPlayer ? "ALERT" : "patrol",
                    static_cast<double>(fpsSmoothed));
            fpsText->SetText(Spark::Utf8String(hud.c_str()));
        }
        (void)dt;
    }

void Maze3DDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        UpdateMazeSkyTransform();

        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

        const Spark::Matrix4 proj =
                Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 400.0F);
        const Spark::Matrix4 view = rig.ViewMatrix();
        const Spark::Matrix4 viewProj = proj * view;

        Spark::SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = rig.CameraWorldPosition();
        params.lightDirectionWorld = Spark::Vector3{0.32F, 0.86F, 0.38F}.Normalized();
        params.lightColor = {1.0F, 0.96F, 0.9F};
        params.lightIntensity = 0.92F;
        params.ambientColor = {0.09F, 0.10F, 0.12F};
        params.lightingProfile = Spark::SceneLightingProfile::NightInterior;
        params.useTimeOfDay = true;
        params.timeOfDay = 0.06F;

        params.draws.Clear();
        params.sceneTextures.Clear();
        params.pointLights.Clear();
        params.particles.Clear();
        params.sprites.Clear();
        params.screenRects.Clear();
        params.screenTexts.Clear();
        params.screenOverlayRects.Clear();
        params.screenOverlayTexts.Clear();
        params.screenLateRects.Clear();
        params.screenLateTexts.Clear();
        params.uiFont = world.GetUiFont();
        params.uiBoldFont = world.GetUiBoldFont();
        params.draws.Reserve(128);

        {
            const Spark::Vector3 pf = rig.ForwardWorld();
            Spark::Vector3 pr = Spark::Vector3::Cross(Spark::Vector3::UnitY, pf);
            if (pr.LengthSquared() < 1.0e-10F) {
                pr = Spark::Vector3::UnitX;
            } else {
                pr = pr.Normalized();
            }
            const Spark::Vector3 pu = Spark::Vector3::Cross(pf, pr).Normalized();
            params.particleCameraRight = pr;
            params.particleCameraUp = pu;
        }

        scene.ForEachParticleEmitter([&params](const Spark::ParticleEmitterComponent& pe,
                                                    const Spark::Matrix4& /*emitterWorld*/) {
            Spark::Array<Spark::SceneParticleInstance> chunk;
            pe.CollectInstances(chunk);
            for (std::size_t ci = 0; ci < chunk.GetSize(); ++ci) {
                if (params.particles.GetSize() >= Spark::SceneRenderParams::MaxParticles) {
                    return;
                }
                params.particles.PushBack(chunk[ci]);
            }
        });

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
        drawList.Reserve(160);

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
            item.metallic = 0.0F;
            item.roughness = 0.5F;
            item.emissiveColor = Spark::Vector3::Zero;
            item.emissiveIntensity = 0.0F;
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

        scene.ForEachSkinnedDrawable([&](Spark::GameObject* obj, const Spark::SkinnedMeshComponent& smc,
                                              const Spark::MaterialComponent* mat, const Spark::AnimatorComponent* anim,
                                              const Spark::Matrix4& world) {
            if (rig.mode == Spark::CharacterCameraMode::FirstPerson && playerGo != nullptr && obj != nullptr
                    && obj->GetParent() == playerGo) {
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

void Maze3DDemo::ApplyMazeSkyVisuals()
{
        if (mazeSkyComp == nullptr || mazeSkyMeshComp == nullptr || mazeSkyMat == nullptr) {
            return;
        }
        mazeSkyComp->SetSkyMode(Spark::SceneSkyMode::Box);
        if (mazeSkyHasEquirect && skyEquirectTex) {
            mazeSkyComp->SetTint(Spark::Vector3::One);
            mazeSkyMeshComp->SetAlbedo(Spark::Vector3::One);
            mazeSkyMat->SetBaseColorTexture(skyEquirectTex);
        } else {
            mazeSkyComp->SetTint({0.22F, 0.34F, 0.58F});
            mazeSkyMeshComp->SetAlbedo(mazeSkyComp->GetTint());
            mazeSkyMat->SetBaseColorTexture(Spark::SharedPtr<Spark::Texture2D>{});
        }
    }

void Maze3DDemo::UpdateMazeSkyTransform()
{
        if (mazeSkyTransform == nullptr) {
            return;
        }
        mazeSkyTransform->SetTranslation(rig.CameraWorldPosition());
        mazeSkyTransform->SetRotation(Spark::Quaternion::Identity);
        mazeSkyTransform->SetUniformScale(98.0F);
    }
}  // namespace Spark
