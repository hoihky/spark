#include "spark/demo/BroadPhase2DDemo.hpp"
#include "spark/demo/DemoAssetLoad.hpp"

namespace Spark {
namespace {

constexpr std::uint32_t kTinyDungeonTileCount =
        DemoAssets::kKenneyTinyDungeonAtlasCols * DemoAssets::kKenneyTinyDungeonAtlasRows;

constexpr std::uint32_t kTinyDungeonPlayerTile = 24U;
constexpr std::uint32_t kTinyDungeonGemTile = 29U;

[[nodiscard]] bool MazeCellIsSolidWall(
        const Array<std::uint8_t>& mazeCells, int mazeW, int mazeH, int x, int y) noexcept {
    if (x < 0 || y < 0 || x >= mazeW || y >= mazeH) {
        return true;
    }
    return mazeCells[static_cast<std::size_t>(y * mazeW + x)] != 0;
}

[[nodiscard]] std::uint32_t TinyDungeonWallAutotileSpark(
        const Array<std::uint8_t>& mazeCells, int mazeW, int mazeH, int x, int y) noexcept {
    const bool wallN = MazeCellIsSolidWall(mazeCells, mazeW, mazeH, x, y + 1);
    const bool wallE = MazeCellIsSolidWall(mazeCells, mazeW, mazeH, x + 1, y);
    const bool wallS = MazeCellIsSolidWall(mazeCells, mazeW, mazeH, x, y - 1);
    const bool wallW = MazeCellIsSolidWall(mazeCells, mazeW, mazeH, x - 1, y);
    const unsigned openN = wallN ? 0U : 1U;
    const unsigned openE = wallE ? 0U : 2U;
    const unsigned openS = wallS ? 0U : 4U;
    const unsigned openW = wallW ? 0U : 8U;
    const unsigned m = openN | openE | openS | openW;
    static constexpr std::uint32_t kByOpenMask[16] = {
            118U, 106U, 117U, 107U, 130U, 118U, 131U, 117U, 119U, 105U, 118U, 130U,
            129U, 119U, 106U, 118U,
    };
    return kByOpenMask[m];
}

[[nodiscard]] Spark::Vector4 TinyDungeonTileUv(const std::uint32_t linearTileIndex) noexcept {
    return DemoAssets::KenneyTinyDungeonTileUv(linearTileIndex);
}

struct MazeIJ {
    int i = 0;
    int j = 0;
};

void ShuffleMazeCells(Array<MazeIJ>& cells, unsigned seed) noexcept {
    for (std::size_t n = cells.GetSize(); n > 1U; --n) {
        seed = seed * 1664525U + 1013904223U;
        const std::size_t k = static_cast<std::size_t>(seed % static_cast<unsigned>(n));
        Swap(cells[k], cells[n - 1U]);
    }
}

void GenerateMazeOddGrid(int w, int h, Array<std::uint8_t>& outCells) {
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

    Array<MazeIJ> stack;
    stack.PushBack(MazeIJ{1, 1});
    outCells[static_cast<std::size_t>(1 * w + 1)] = 0;

    while (!stack.IsEmpty()) {
        const MazeIJ cur = stack[stack.GetSize() - 1];
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
            stack.PushBack(MazeIJ{nx, ny});
            advanced = true;
            break;
        }
        if (!advanced) {
            stack.PopBack();
        }
    }
}

void BuildPhysicalMazeWideFloorsThinWalls(
        const Array<std::uint8_t>& logic,
        int lw,
        int lh,
        int k,
        Array<std::uint8_t>& phys) noexcept {
    const int stride = k + 1;
    const int pw = lw * stride - 1;
    const int ph = lh * stride - 1;
    phys.Resize(static_cast<std::size_t>(pw * ph));
    for (std::size_t i = 0; i < phys.GetSize(); ++i) {
        phys[i] = 1;
    }
    for (int ly = 0; ly < lh; ++ly) {
        for (int lx = 0; lx < lw; ++lx) {
            if (logic[static_cast<std::size_t>(ly * lw + lx)] != 0) {
                continue;
            }
            const int bx = lx * stride;
            const int by = ly * stride;
            for (int dy = 0; dy < k; ++dy) {
                for (int dx = 0; dx < k; ++dx) {
                    phys[static_cast<std::size_t>((by + dy) * pw + (bx + dx))] = 0;
                }
            }
        }
    }
    for (int ly = 0; ly < lh; ++ly) {
        for (int lx = 0; lx < lw; ++lx) {
            if (logic[static_cast<std::size_t>(ly * lw + lx)] != 0) {
                continue;
            }
            if (lx + 1 < lw && logic[static_cast<std::size_t>(ly * lw + lx + 1)] == 0) {
                const int px = lx * stride + k;
                for (int py = ly * stride; py < ly * stride + k; ++py) {
                    phys[static_cast<std::size_t>(py * pw + px)] = 0;
                }
            }
            if (ly + 1 < lh && logic[static_cast<std::size_t>((ly + 1) * lw + lx)] == 0) {
                const int py = ly * stride + k;
                for (int px = lx * stride; px < lx * stride + k; ++px) {
                    phys[static_cast<std::size_t>(py * pw + px)] = 0;
                }
            }
        }
    }
}

[[nodiscard]] bool PhysicalCellReservedForSpawn(
        int px,
        int py,
        int lw,
        int lh,
        int k,
        int stride,
        const Array<std::uint8_t>& logic) noexcept {
    static constexpr int rx[3] = {1, 1, 2};
    static constexpr int ry[3] = {1, 2, 1};
    for (int i = 0; i < 3; ++i) {
        const int lx = rx[i];
        const int ly = ry[i];
        if (px >= lx * stride && px < lx * stride + k && py >= ly * stride && py < ly * stride + k) {
            return true;
        }
    }
    if (lw > 2 && logic[static_cast<std::size_t>(1 * lw + 1)] == 0 && logic[static_cast<std::size_t>(1 * lw + 2)] == 0) {
        const int pxConn = 1 * stride + k;
        if (px == pxConn && py >= 1 * stride && py < 1 * stride + k) {
            return true;
        }
    }
    if (lh > 2 && logic[static_cast<std::size_t>(1 * lw + 1)] == 0 && logic[static_cast<std::size_t>(2 * lw + 1)] == 0) {
        const int pyConn = 1 * stride + k;
        if (py == pyConn && px >= 1 * stride && px < 1 * stride + k) {
            return true;
        }
    }
    return false;
}

}  // namespace

void BroadPhase2DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        gemObjects.Clear();
        staticColliders.Clear();
        broadGrid.Clear();
        queryScratch.Clear();
        if (mazeAudioEngine != nullptr) {
            mazeAudioEngine->ClearBackgroundMusic();
            mazeAudioEngine = nullptr;
        }
        wallCount = 0;
        lastBroadCandidates = 0;
        lastNarrowHits = 0;
        gemsCollected = 0;
        gemsTotal = 0;
        sceneTime = 0.0F;

        Array<std::uint8_t> logicCells;
        GenerateMazeOddGrid(kMazeLogicalW, kMazeLogicalH, logicCells);
        Array<std::uint8_t> cells;
        BuildPhysicalMazeWideFloorsThinWalls(
                logicCells, kMazeLogicalW, kMazeLogicalH, kCorridorFloorCells, cells);

        Spark::Texture2D dungeonAtlasCpu{};
        if (!DemoAssets::TryLoadKenneyTinyDungeonAtlas(dungeonAtlasCpu)) {
            dungeonAtlasCpu = Spark::Texture2D::CreateCheckerboard(
                    128,
                    24,
                    Spark::Vector3{0.96F, 0.97F, 1.0F},
                    Spark::Vector3{0.22F, 0.38F, 0.78F});
            dungeonAtlasCpu.GetName() = Spark::Utf8String("Maze2DAtlasFallback");
            dungeonAtlasCpu.SetSceneUploadNearest(true);
        }
        dungeonAtlasTex = Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(dungeonAtlasCpu));
        w.RegisterTexture(dungeonAtlasTex, "spark/maze2d/kenney_tiny_dungeon");

        const float originX = -0.5F * static_cast<float>(kMazeW) * kCellWorld;
        const float originY = -0.5F * static_cast<float>(kMazeH) * kCellWorld;

        for (int y = 0; y < kMazeH; ++y) {
            for (int x = 0; x < kMazeW; ++x) {
                if (cells[static_cast<std::size_t>(y * kMazeW + x)] == 0) {
                    continue;
                }
                Spark::GameObject* g = w.CreateGameObject();
                g->GetName() = Spark::Utf8String("MazeWall");
                Spark::TransformComponent* tr = g->AddComponent<Spark::TransformComponent>();
                const float cx = originX + (static_cast<float>(x) + 0.5F) * kCellWorld;
                const float cy = originY + (static_cast<float>(y) + 0.5F) * kCellWorld;
                tr->SetTranslation({cx, cy, 0.01F + 0.0002F * static_cast<float>(x + y)});
                tr->SetScale({kCellWorld, kCellWorld, 1.0F});
                g->AddComponent<Spark::SpriteComponent>(
                        dungeonAtlasTex,
                        Spark::Vector4{0.92F, 0.92F, 0.96F, 1.0F},
                        TinyDungeonTileUv(TinyDungeonWallAutotileSpark(cells, kMazeW, kMazeH, x, y)),
                        4 + ((x + y * 3) % 40));
                g->AddComponent<Spark::BoxCollider2DComponent>();
                roots.PushBack(g);
                ++wallCount;
            }
        }

        playerGo = w.CreateGameObject();
        playerGo->GetName() = Spark::Utf8String("MazePlayer");
        playerTr = playerGo->AddComponent<Spark::TransformComponent>();
        static constexpr int kSpawnLogicalX = 1;
        static constexpr int kSpawnLogicalY = 1;
        const float spawnCellCx =
                static_cast<float>(kSpawnLogicalX * kMazeStride) + 0.5F * static_cast<float>(kCorridorFloorCells);
        const float spawnCellCy =
                static_cast<float>(kSpawnLogicalY * kMazeStride) + 0.5F * static_cast<float>(kCorridorFloorCells);
        const float px = originX + spawnCellCx * kCellWorld;
        const float py = originY + spawnCellCy * kCellWorld;
        playerTr->SetTranslation({px, py, 0.06F});
        playerTr->SetScale({0.74F * kCellWorld, 0.74F * kCellWorld, 1.0F});
        playerGo->AddComponent<Spark::SpriteComponent>(
                dungeonAtlasTex,
                Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                TinyDungeonTileUv(kTinyDungeonPlayerTile),
                6000);
        playerGo->AddComponent<Spark::BoxCollider2DComponent>();
        playerRb = playerGo->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);
        playerRb->SetVelocity(Spark::Vector2::Zero);

        Array<MazeIJ> floorCells;
        for (int y = 0; y < kMazeH; ++y) {
            for (int x = 0; x < kMazeW; ++x) {
                if (cells[static_cast<std::size_t>(y * kMazeW + x)] != 0) {
                    continue;
                }
                if (PhysicalCellReservedForSpawn(
                            x,
                            y,
                            kMazeLogicalW,
                            kMazeLogicalH,
                            kCorridorFloorCells,
                            kMazeStride,
                            logicCells)) {
                    continue;
                }
                floorCells.PushBack(MazeIJ{x, y});
            }
        }
        ShuffleMazeCells(floorCells, static_cast<unsigned>(kMazeW * 49999 + kMazeH * 131U + 17U));
        constexpr int kMaxGems = 28;
        gemsTotal = static_cast<int>(floorCells.GetSize());
        if (gemsTotal > kMaxGems) {
            gemsTotal = kMaxGems;
        }
        for (int gi = 0; gi < gemsTotal; ++gi) {
            const MazeIJ c = floorCells[static_cast<std::size_t>(gi)];
            Spark::GameObject* gem = w.CreateGameObject();
            gem->GetName() = Spark::Utf8String("MazeGem");
            Spark::TransformComponent* gtr = gem->AddComponent<Spark::TransformComponent>();
            const float gx = originX + (static_cast<float>(c.i) + 0.5F) * kCellWorld;
            const float gy = originY + (static_cast<float>(c.j) + 0.5F) * kCellWorld;
            gtr->SetTranslation({gx, gy, 0.055F + 0.0001F * static_cast<float>(gi)});
            gtr->SetScale({0.34F * kCellWorld, 0.34F * kCellWorld, 1.0F});
            const float hue = static_cast<float>(gi) * 0.37F;
            const Spark::Vector3 rgb{
                    0.35F + 0.45F * std::fabs(std::sin(hue)),
                    0.55F + 0.35F * std::fabs(std::sin(hue + 2.1F)),
                    0.85F + 0.15F * std::fabs(std::sin(hue + 4.2F))};
            gem->AddComponent<Spark::SpriteComponent>(
                    dungeonAtlasTex,
                    Spark::Vector4{1.0F, 1.0F, 1.0F, 1.0F},
                    TinyDungeonTileUv(kTinyDungeonGemTile),
                    6000);
            const float pulseHz = 1.1F + 0.11F * static_cast<float>(gi % 7);
            const float emitStr = 1.35F + 0.08F * static_cast<float>(gi % 5);
            gem->AddComponent<Spark::SpriteLighting2DComponent>(
                    SpriteLighting2DMode::PulseEmission,
                    Spark::Vector4{rgb.x * 1.2F, rgb.y * 1.2F, rgb.z * 1.15F, pulseHz},
                    Spark::Vector4{emitStr, 0.42F, 0.0F, 0.0F});
            gemObjects.PushBack(gem);
        }

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("MazeFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*fpsText, false);
        fpsText->SetText(Spark::Utf8String("Tiny Dungeon — gems — HingeJoint2D swing near spawn — WASD — ESC"));
        roots.PushBack(playerGo);
        roots.PushBack(fpsHudObject);

        {
            const float hingeX = px + 2.8F * kCellWorld;
            const float hingeY = py;
            Spark::GameObject* hingeAnchor = w.CreateGameObject();
            hingeAnchor->GetName() = Spark::Utf8String("Maze2DHingeAnchor");
            Spark::TransformComponent* anchorTr = hingeAnchor->AddComponent<Spark::TransformComponent>();
            anchorTr->SetTranslation({hingeX, hingeY, 0.04F});
            anchorTr->SetScale({0.35F * kCellWorld, 0.35F * kCellWorld, 1.0F});
            hingeAnchor->AddComponent<Spark::SpriteComponent>(
                    dungeonAtlasTex,
                    Spark::Vector4{0.72F, 0.72F, 0.78F, 1.0F},
                    TinyDungeonTileUv(118U),
                    5000);
            hingeAnchor->AddComponent<Spark::BoxCollider2DComponent>();
            hingeAnchor->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Static, 1.0F);
            roots.PushBack(hingeAnchor);

            Spark::GameObject* swingPlat = w.CreateGameObject();
            swingPlat->GetName() = Spark::Utf8String("Maze2DSwingPlat");
            Spark::TransformComponent* swingTr = swingPlat->AddComponent<Spark::TransformComponent>();
            swingTr->SetTranslation({hingeX, hingeY - 1.6F * kCellWorld, 0.05F});
            swingTr->SetScale({1.4F * kCellWorld, 0.28F * kCellWorld, 1.0F});
            swingPlat->AddComponent<Spark::SpriteComponent>(
                    dungeonAtlasTex,
                    Spark::Vector4{0.55F, 0.82F, 0.95F, 1.0F},
                    TinyDungeonTileUv(kTinyDungeonGemTile),
                    5100);
            swingPlat->AddComponent<Spark::BoxCollider2DComponent>();
            Spark::Rigidbody2DComponent* swingRb =
                    swingPlat->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);
            swingRb->SetGravityScale(0.0F);
            Spark::HingeJoint2DComponent* hinge =
                    swingPlat->AddComponent<Spark::HingeJoint2DComponent>(hingeAnchor);
            hinge->SetLocalAnchorA({0.0F, 0.5F * kCellWorld});
            hinge->SetLocalAnchorB({0.0F, -0.5F * kCellWorld});
            hinge->SetStiffness(0.72F);
            roots.PushBack(swingPlat);
        }

        const float broadCell = std::max(16.0F, kCellWorld * 4.0F);
        Spark::ColliderBakePipeline2D::GetDefault().Rebuild(w, broadCell, staticColliders, broadGrid);

        PhysicsWorld2DSettings& phys = physics.GetWorld2D().GetSettings();
        phys.gravityY = 0.0F;
        phys.maxFallSpeed = 500.0F;
        phys.resolveDynamicVsDynamic = true;
        phys.jointIterations = 6;

        camera.position = {px, py, 0.0F};
        camera.halfExtentY = kCameraHalfExtentInCells * kCellWorld;
        camera.rotationRad = 0.0F;

        context.GetInput().SetCursorCaptured(false);

        mazeAudioEngine = context.TryGetSoundEngine();
        if (mazeAudioEngine != nullptr && mazeAudioEngine->IsRunning()) {
            auto clip = TryLoadSoundClipFromBundledAsset("assets/audio/castle.wav");
            if (!clip) {
                clip = SoundClip::CreateSimpleAmbienceLoop();
            }
            mazeAudioEngine->SetBackgroundMusic(clip, 0.26F, true);
        }
    }

void BroadPhase2DDemo::Unload(Spark::GameWorld& w)
{
        if (mazeAudioEngine != nullptr) {
            mazeAudioEngine->ClearBackgroundMusic();
            mazeAudioEngine = nullptr;
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
        dungeonAtlasTex.Reset();
        staticColliders.Clear();
        broadGrid.Clear();
        queryScratch.Clear();
        playerGo = nullptr;
        playerTr = nullptr;
        playerRb = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
    }

void BroadPhase2DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world)
{
        sceneTime = timing.totalTimeSeconds;
        Spark::IInput& in = context.GetInput();
        const float dt = timing.deltaTimeSeconds;

        if (playerRb != nullptr && playerTr != nullptr) {
            float mx = 0.0F;
            float my = 0.0F;
            if (in.IsKeyDown(GLFW_KEY_A) || in.IsKeyDown(GLFW_KEY_LEFT)) {
                mx -= 1.0F;
            }
            if (in.IsKeyDown(GLFW_KEY_D) || in.IsKeyDown(GLFW_KEY_RIGHT)) {
                mx += 1.0F;
            }
            if (in.IsKeyDown(GLFW_KEY_W) || in.IsKeyDown(GLFW_KEY_UP)) {
                my += 1.0F;
            }
            if (in.IsKeyDown(GLFW_KEY_S) || in.IsKeyDown(GLFW_KEY_DOWN)) {
                my -= 1.0F;
            }
            const float lenSq = mx * mx + my * my;
            if (lenSq > 1.0e-6F) {
                const float inv = 1.0F / std::sqrt(lenSq);
                mx *= inv;
                my *= inv;
            }
            constexpr float kMoveSpeedPerCell = 11.0F;
            const float moveSpeed = kMoveSpeedPerCell * kCellWorld;
            playerRb->SetVelocity({mx * moveSpeed, my * moveSpeed});

            physics.Simulate2D(world, timing);

            const Spark::BoxCollider2DComponent* pCol = playerGo->GetComponent<Spark::BoxCollider2DComponent>();
            if (pCol != nullptr) {
                CollisionAabb2 playerBox{};
                ComputeBoxCollider2WorldAabb(*playerGo, *pCol, playerBox);
                broadGrid.QueryUniquePayloadIndices(playerBox, queryScratch);
                lastBroadCandidates = static_cast<std::uint32_t>(queryScratch.GetSize());
                std::uint32_t narrowHits = 0;
                for (std::size_t i = 0; i < queryScratch.GetSize(); ++i) {
                    const std::uint32_t idx = queryScratch[i];
                    if (idx < staticColliders.GetSize() &&
                        staticColliders[idx].OverlapsAabb(playerBox)) {
                        ++narrowHits;
                    }
                }
                lastNarrowHits = narrowHits;
            }

            const Spark::Vector3 p = playerTr->GetLocalTransform().translation;
            const float follow = std::min(1.0F, 8.0F * dt);
            camera.position.x += (p.x - camera.position.x) * follow;
            camera.position.y += (p.y - camera.position.y) * follow;

            const float collectRadius = 0.48F * kCellWorld;
            const float cr2 = collectRadius * collectRadius;
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
                if (dx * dx + dy * dy <= cr2) {
                    world.DestroyGameObject(gem);
                    gemObjects.RemoveAt(gi);
                    ++gemsCollected;
                    DemoPlayProceduralClip(context, DemoSfx::ClipGemCollect(), 0.95F);
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
            const std::string hud = std::format(
                    "Maze {}×{} ({}×{} floor×{}) — {} walls — gems {}/{} — {:.0f} FPS — hash {} / narrow {} — "
                    "HingeJoint2D — WASD — ESC",
                    kMazeW,
                    kMazeH,
                    kMazeLogicalW,
                    kMazeLogicalH,
                    kCorridorFloorCells,
                    wallCount,
                    gemsCollected,
                    gemsTotal,
                    static_cast<double>(fpsSmoothed),
                    lastBroadCandidates,
                    lastNarrowHits);
            fpsText->SetText(Spark::Utf8String(hud.c_str()));
        }
        (void)dt;
    }

void BroadPhase2DDemo::Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        if (fbW <= 0) {
            fbW = 1;
        }
        if (fbH <= 0) {
            fbH = 1;
        }
        const Spark::Matrix4 viewProj =
                camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
        Spark::Vector3 pr{};
        Spark::Vector3 pu{};
        camera.BillboardBasisWorld(pr, pu);
        Spark::SubmitStandardLitSceneFromWorld(
                world,
                context,
                viewProj,
                camera.position,
                Spark::Vector3{0.22F, 0.78F, 0.36F}.Normalized(),
                Spark::Vector3{1.0F, 1.0F, 1.0F},
                0.82F,
                Spark::Vector3{0.08F, 0.09F, 0.14F},
                false,
                pr,
                pu,
                sceneTime,
                Spark::SceneSpriteSortMode::SortOrderThenWorldY);
    }
}  // namespace Spark
