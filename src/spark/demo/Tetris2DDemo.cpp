#include "spark/demo/Tetris2DDemo.hpp"

#include "spark/ecs/components/BlendModeComponent.hpp"
#include "spark/ecs/components/SpriteComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/render/SceneBlendMode.hpp"

namespace Spark {
namespace Detail {

[[nodiscard]] Spark::SharedPtr<Spark::Texture2D> MakeTetrisBlockAtlas() {
    constexpr std::uint32_t tw = 16;
    constexpr std::uint32_t au = 4;
    constexpr std::uint32_t av = 2;
    constexpr std::uint32_t w = au * tw;
    constexpr std::uint32_t h = av * tw;
    Spark::Texture2D tex(Spark::Utf8String("TetrisBlockAtlas"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    const Spark::Vector3 palette[8] = {
            {0.12F, 0.12F, 0.14F},
            {0.15F, 0.92F, 0.95F},
            {0.98F, 0.92F, 0.22F},
            {0.72F, 0.28F, 0.95F},
            {0.25F, 0.95F, 0.35F},
            {0.95F, 0.28F, 0.28F},
            {0.28F, 0.38F, 0.98F},
            {0.98F, 0.62F, 0.22F},
    };
    for (std::uint32_t ty = 0; ty < av; ++ty) {
        for (std::uint32_t tx = 0; tx < au; ++tx) {
            const std::uint32_t id = ty * au + tx;
            const Spark::Vector3& c = palette[std::min<std::uint32_t>(id, 7U)];
            for (std::uint32_t py = 0; py < tw; ++py) {
                for (std::uint32_t px0 = 0; px0 < tw; ++px0) {
                    const std::uint32_t gx = tx * tw + px0;
                    const std::uint32_t gy = ty * tw + py;
                    const std::size_t di = (static_cast<std::size_t>(gy) * w + gx) * 4U;
                    px[di] = static_cast<std::uint8_t>(std::min(255.0F, c.x * 255.0F));
                    px[di + 1U] = static_cast<std::uint8_t>(std::min(255.0F, c.y * 255.0F));
                    px[di + 2U] = static_cast<std::uint8_t>(std::min(255.0F, c.z * 255.0F));
                    px[di + 3U] = 255;
                }
            }
        }
    }
    tex.SetPixels(w, h, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

}  // namespace Detail

void Tetris2DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        atlasTex = Detail::MakeTetrisBlockAtlas();
        w.RegisterTexture(atlasTex, "spark/tetris/blocks");

        boardGo = w.CreateGameObject();
        boardGo->GetName() = Spark::Utf8String("TetrisBoard");
        boardGo->AddComponent<Spark::TransformComponent>();
        tilemap = boardGo->AddComponent<Spark::TilemapComponent>(
                atlasTex,
                static_cast<std::uint32_t>(kCols),
                static_cast<std::uint32_t>(kRows),
                4,
                2,
                kTileWorld,
                10);
        roots.PushBack(boardGo);

        boardPadGo = w.CreateGameObject();
        boardPadGo->GetName() = Spark::Utf8String("TetrisBoardPad");
        {
            Spark::TransformComponent* tr = boardPadGo->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({static_cast<float>(kCols) * 0.5F * kTileWorld, static_cast<float>(kRows) * 0.5F * kTileWorld, -0.02F});
            tr->SetScale({static_cast<float>(kCols) * kTileWorld + 0.35F, static_cast<float>(kRows) * kTileWorld + 0.35F, 1.0F});
        }
        boardPadGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Multiply);
        boardPadGo->AddComponent<Spark::SpriteComponent>(
                atlasTex,
                Spark::Vector4{0.22F, 0.24F, 0.30F, 0.88F},
                Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                5);
        roots.PushBack(boardPadGo);

        ghostCells.Clear();
        ghostCells.Reserve(4);
        for (int gi = 0; gi < 4; ++gi) {
            Spark::GameObject* ghost = w.CreateGameObject();
            ghost->GetName() = Spark::Utf8String("TetrisGhostCell");
            ghost->AddComponent<Spark::TransformComponent>();
            ghost->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Multiply);
            ghost->AddComponent<Spark::SpriteComponent>(
                    atlasTex,
                    Spark::Vector4{0.55F, 0.58F, 0.65F, 0.72F},
                    TileUv(1),
                    8);
            ghostCells.PushBack(ghost);
            roots.PushBack(ghost);
        }

        lineFlashGo = w.CreateGameObject();
        lineFlashGo->GetName() = Spark::Utf8String("TetrisLineFlash");
        {
            Spark::TransformComponent* tr = lineFlashGo->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({static_cast<float>(kCols) * 0.5F * kTileWorld, 0.0F, 0.05F});
            tr->SetScale({static_cast<float>(kCols) * kTileWorld, kTileWorld * 1.2F, 1.0F});
        }
        lineFlashGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
        lineFlashGo->AddComponent<Spark::SpriteComponent>(
                atlasTex,
                Spark::Vector4{0.35F, 0.95F, 1.0F, 0.0F},
                Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                900);
        roots.PushBack(lineFlashGo);

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("TetrisFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(12.0F, 12.0F);
        fpsText->SetFontSizePixels(20.0F);
        fpsText->SetColor({0.95F, 0.98F, 1.0F});
        fpsText->SetText(Spark::Utf8String("Tetris — multiply ghost · additive line clear · ←→ ↓ ↑/X rotate · Space slam"));
        roots.PushBack(fpsHudObject);

        camera.position = {static_cast<float>(kCols) * 0.5F * kTileWorld, static_cast<float>(kRows) * 0.5F * kTileWorld, 0.0F};
        camera.halfExtentY = static_cast<float>(kRows) * 0.55F * kTileWorld;
        camera.rotationRad = 0.0F;

        RandSeed(timingHackU32(context));
        ResetGame();
        context.GetInput().SetCursorCaptured(false);
    }

void Tetris2DDemo::Unload(Spark::GameWorld& w)
{
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        atlasTex.Reset();
        boardGo = nullptr;
        tilemap = nullptr;
        boardPadGo = nullptr;
        ghostCells.Clear();
        lineFlashGo = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
    }

void Tetris2DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        Spark::IInput& in = context.GetInput();
        const float dt = timing.deltaTimeSeconds;
        if (lineFlashT > 0.0F) {
            lineFlashT = std::max(0.0F, lineFlashT - dt);
            if (lineFlashGo != nullptr) {
                if (Spark::SpriteComponent* flash = lineFlashGo->GetComponent<Spark::SpriteComponent>()) {
                    const float pulse = lineFlashT / 0.42F;
                    flash->SetTint({0.35F, 0.95F, 1.0F, 0.85F * pulse * pulse});
                }
            }
        }

        if (gameOver) {
            if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
                ResetGame();
            }
            if (fpsText != nullptr) {
                fpsText->SetText(Spark::Utf8String(
                        std::format(
                                "GAME OVER — lines {} score {} — R restart · ESC menu",
                                static_cast<int>(linesCleared),
                                static_cast<int>(score))
                                .c_str()));
            }
            return;
        }

        if (in.IsKeyPressedThisFrame(GLFW_KEY_LEFT)) {
            static_cast<void>(TryMove(-1, 0));
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_RIGHT)) {
            static_cast<void>(TryMove(1, 0));
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_DOWN)) {
            if (TryMove(0, -1)) {
                score += 1;
            }
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_UP) || in.IsKeyPressedThisFrame(GLFW_KEY_X)) {
            TryRotate(context);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
            while (TryMove(0, -1)) {
                score += 2;
            }
            LockPiece(context);
        }

        fallAccum += dt;
        const float period = std::max(0.08F, 0.55F - static_cast<float>(level) * 0.045F);
        while (fallAccum >= period) {
            fallAccum -= period;
            if (!TryMove(0, -1)) {
                LockPiece(context);
                break;
            }
        }

        UpdateGhostSprites();
        UpdateBoardPad();

        if (fpsText != nullptr) {
            const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            fpsText->SetText(Spark::Utf8String(
                    std::format(
                            "Tetris — {:.0f} FPS — score {} · lines {} · lvl {} · ESC menu",
                            static_cast<double>(fpsSmoothed),
                            static_cast<int>(score),
                            static_cast<int>(linesCleared),
                            static_cast<int>(level))
                            .c_str()));
        }
    }

void Tetris2DDemo::Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context)
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
                Spark::Vector3{0.35F, 0.88F, 0.42F}.Normalized(),
                Spark::Vector3{0.95F, 0.97F, 1.0F},
                0.75F,
                Spark::Vector3{0.08F, 0.09F, 0.12F},
                false,
                pr,
                pu,
                0.0F);
    }

std::uint32_t Tetris2DDemo::timingHackU32(Spark::IEngineContext& context)
{
        int w = 0;
        int h = 0;
        context.GetFramebufferSize(w, h);
        return static_cast<std::uint32_t>(w * 7919 + h * 65537 + 1);
    }

void Tetris2DDemo::RandSeed(std::uint32_t s) noexcept
{ rng = s != 0 ? s : 1U; }

[[nodiscard]] std::uint32_t Tetris2DDemo::RandU32() noexcept
{
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }

[[nodiscard]] int Tetris2DDemo::RandPieceKind() noexcept
{ return static_cast<int>(RandU32() % 7U); }

void Tetris2DDemo::ResetGame()
{
        grid.Clear();
        grid.Resize(static_cast<std::size_t>(kCols) * static_cast<std::size_t>(kRows));
        score = 0;
        linesCleared = 0;
        level = 1;
        fallAccum = 0.0F;
        gameOver = false;
        SpawnPiece();
        PushBoardToTilemap();
    }

void Tetris2DDemo::SpawnPiece()
{
        curKind = RandPieceKind();
        curRot = 0;
        anchorX = 3;
        anchorY = kRows - 4;
        if (!Fits(curKind, curRot, anchorX, anchorY)) {
            gameOver = true;
        }
    }

[[nodiscard]] bool Tetris2DDemo::Fits(int kind, int rot, int ax, int ay) const noexcept
{
        for (int i = 0; i < 4; ++i) {
            const int ox = Detail::kPieceCells[static_cast<std::size_t>(kind)][static_cast<std::size_t>(rot & 3)][static_cast<std::size_t>(i)][0];
            const int oy = Detail::kPieceCells[static_cast<std::size_t>(kind)][static_cast<std::size_t>(rot & 3)][static_cast<std::size_t>(i)][1];
            const int gx = ax + static_cast<int>(ox);
            const int gy = ay + static_cast<int>(oy);
            if (gx < 0 || gx >= kCols || gy < 0 || gy >= kRows) {
                return false;
            }
            if (grid[static_cast<std::size_t>(gy) * static_cast<std::size_t>(kCols) + static_cast<std::size_t>(gx)] != 0) {
                return false;
            }
        }
        return true;
    }

[[nodiscard]] bool Tetris2DDemo::TryMove(int dx, int dy) noexcept
{
        if (gameOver) {
            return false;
        }
        const int nx = anchorX + dx;
        const int ny = anchorY + dy;
        if (!Fits(curKind, curRot, nx, ny)) {
            return false;
        }
        anchorX = nx;
        anchorY = ny;
        PushBoardToTilemap();
        return true;
    }

void Tetris2DDemo::TryRotate(Spark::IEngineContext& context) noexcept
{
        if (gameOver) {
            return;
        }
        const int nr = (curRot + 1) & 3;
        if (Fits(curKind, nr, anchorX, anchorY)) {
            curRot = nr;
            PushBoardToTilemap();
            DemoPlayProceduralClip(context, DemoSfx::ClipTetrisRotate(), 0.85F);
            return;
        }
        if (Fits(curKind, nr, anchorX - 1, anchorY)) {
            anchorX -= 1;
            curRot = nr;
            PushBoardToTilemap();
            DemoPlayProceduralClip(context, DemoSfx::ClipTetrisRotate(), 0.85F);
            return;
        }
        if (Fits(curKind, nr, anchorX + 1, anchorY)) {
            anchorX += 1;
            curRot = nr;
            PushBoardToTilemap();
            DemoPlayProceduralClip(context, DemoSfx::ClipTetrisRotate(), 0.85F);
            return;
        }
    }

void Tetris2DDemo::LockPiece(Spark::IEngineContext& context)
{
        for (int i = 0; i < 4; ++i) {
            const int ox = Detail::kPieceCells[static_cast<std::size_t>(curKind)][static_cast<std::size_t>(curRot & 3)][static_cast<std::size_t>(i)][0];
            const int oy = Detail::kPieceCells[static_cast<std::size_t>(curKind)][static_cast<std::size_t>(curRot & 3)][static_cast<std::size_t>(i)][1];
            const int gx = anchorX + ox;
            const int gy = anchorY + oy;
            if (gx >= 0 && gx < kCols && gy >= 0 && gy < kRows) {
                grid[static_cast<std::size_t>(gy) * static_cast<std::size_t>(kCols) + static_cast<std::size_t>(gx)] =
                        static_cast<std::uint8_t>(curKind + 1U);
            }
        }
        const int cleared = ClearLines();
        if (cleared > 0) {
            lineFlashT = 0.42F;
            if (lineFlashGo != nullptr) {
                if (Spark::TransformComponent* flashTr = lineFlashGo->GetComponent<Spark::TransformComponent>()) {
                    const Spark::Vector3 t = flashTr->GetLocalTransform().translation;
                    flashTr->SetTranslation({t.x, (static_cast<float>(anchorY) + 0.5F) * kTileWorld, t.z});
                }
            }
            const float hz = 340.0F + static_cast<float>(cleared) * 95.0F;
            const float dur = 0.078F + 0.012F * static_cast<float>(cleared);
            DemoPlayProceduralClip(context, SoundClip::CreateToneBlip(hz, dur, 0.26F), 1.0F);
        } else {
            DemoPlayProceduralClip(context, DemoSfx::ClipTetrisLock(), 0.72F);
        }
        SpawnPiece();
        PushBoardToTilemap();
    }

[[nodiscard]] int Tetris2DDemo::ClearLines()
{
        int cleared = 0;
        for (int y = 0; y < kRows; ++y) {
            bool full = true;
            for (int x = 0; x < kCols; ++x) {
                if (grid[static_cast<std::size_t>(y) * static_cast<std::size_t>(kCols) + static_cast<std::size_t>(x)] == 0) {
                    full = false;
                    break;
                }
            }
            if (!full) {
                continue;
            }
            ++cleared;
            for (int yy = y; yy < kRows - 1; ++yy) {
                for (int x = 0; x < kCols; ++x) {
                    grid[static_cast<std::size_t>(yy) * static_cast<std::size_t>(kCols) + static_cast<std::size_t>(x)] =
                            grid[static_cast<std::size_t>(yy + 1) * static_cast<std::size_t>(kCols) + static_cast<std::size_t>(x)];
                }
            }
            for (int x = 0; x < kCols; ++x) {
                grid[static_cast<std::size_t>(kRows - 1) * static_cast<std::size_t>(kCols) + static_cast<std::size_t>(x)] = 0;
            }
            --y;
        }
        if (cleared > 0) {
            linesCleared += cleared;
            static constexpr int kLineScore[] = {0, 100, 300, 500, 800};
            score += kLineScore[std::min(cleared, 4)];
            level = 1 + linesCleared / 10;
        }
        return cleared;
    }

void Tetris2DDemo::PushBoardToTilemap()
{
        if (tilemap == nullptr) {
            return;
        }
        for (int y = 0; y < kRows; ++y) {
            for (int x = 0; x < kCols; ++x) {
                std::uint8_t v = grid[static_cast<std::size_t>(y) * static_cast<std::size_t>(kCols) + static_cast<std::size_t>(x)];
                if (!gameOver && v == 0) {
                    for (int i = 0; i < 4; ++i) {
                        const int ox = Detail::kPieceCells[static_cast<std::size_t>(curKind)][static_cast<std::size_t>(curRot & 3)][static_cast<std::size_t>(i)][0];
                        const int oy = Detail::kPieceCells[static_cast<std::size_t>(curKind)][static_cast<std::size_t>(curRot & 3)][static_cast<std::size_t>(i)][1];
                        if (anchorX + ox == x && anchorY + oy == y) {
                            v = static_cast<std::uint8_t>(curKind + 1U);
                            break;
                        }
                    }
                }
                if (v == 0) {
                    tilemap->SetTile(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), Spark::TilemapComponent::kEmptyTile);
                } else {
                    tilemap->SetTile(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), static_cast<std::uint16_t>(v));
                }
            }
        }
    }

Spark::Vector4 Tetris2DDemo::TileUv(const std::uint16_t tileId) noexcept
{
        constexpr std::uint32_t kAtlasU = 4U;
        constexpr std::uint32_t kAtlasV = 2U;
        const std::uint32_t id = static_cast<std::uint32_t>(tileId);
        const std::uint32_t tx = id % kAtlasU;
        const std::uint32_t ty = id / kAtlasU;
        const float du = 1.0F / static_cast<float>(kAtlasU);
        const float dv = 1.0F / static_cast<float>(kAtlasV);
        return {static_cast<float>(tx) * du, static_cast<float>(ty) * dv, static_cast<float>(tx + 1U) * du,
                static_cast<float>(ty + 1U) * dv};
    }

void Tetris2DDemo::UpdateBoardPad()
{
        if (boardPadGo == nullptr) {
            return;
        }
        Spark::TransformComponent* tr = boardPadGo->GetComponent<Spark::TransformComponent>();
        if (tr == nullptr) {
            return;
        }
        tr->SetTranslation(
                {static_cast<float>(kCols) * 0.5F * kTileWorld,
                 static_cast<float>(kRows) * 0.5F * kTileWorld,
                 -0.02F});
    }

void Tetris2DDemo::UpdateGhostSprites()
{
        if (gameOver) {
            for (std::size_t i = 0; i < ghostCells.GetSize(); ++i) {
                if (ghostCells[i] != nullptr) {
                    if (Spark::TransformComponent* tr = ghostCells[i]->GetComponent<Spark::TransformComponent>()) {
                        tr->SetTranslation({-20.0F, -20.0F, 0.0F});
                    }
                }
            }
            return;
        }

        int ghostY = anchorY;
        while (Fits(curKind, curRot, anchorX, ghostY - 1)) {
            --ghostY;
        }

        const std::uint16_t tileId = static_cast<std::uint16_t>(curKind + 1U);
        const Spark::Vector4 pieceUv = TileUv(tileId);
        int cellSlot = 0;
        for (int i = 0; i < 4; ++i) {
            const int ox = Detail::kPieceCells[static_cast<std::size_t>(curKind)][static_cast<std::size_t>(curRot & 3)]
                                                 [static_cast<std::size_t>(i)][0];
            const int oy = Detail::kPieceCells[static_cast<std::size_t>(curKind)][static_cast<std::size_t>(curRot & 3)]
                                                 [static_cast<std::size_t>(i)][1];
            const int gx = anchorX + ox;
            const int gy = ghostY + oy;
            if (cellSlot >= static_cast<int>(ghostCells.GetSize()) || ghostCells[static_cast<std::size_t>(cellSlot)] == nullptr) {
                continue;
            }
            Spark::GameObject* ghost = ghostCells[static_cast<std::size_t>(cellSlot)];
            if (Spark::TransformComponent* tr = ghost->GetComponent<Spark::TransformComponent>()) {
                tr->SetTranslation(
                        {(static_cast<float>(gx) + 0.5F) * kTileWorld,
                         (static_cast<float>(gy) + 0.5F) * kTileWorld,
                         0.005F});
                tr->SetUniformScale(kTileWorld);
            }
            if (Spark::SpriteComponent* spr = ghost->GetComponent<Spark::SpriteComponent>()) {
                spr->SetUvRect(pieceUv);
            }
            ++cellSlot;
        }
        for (; cellSlot < static_cast<int>(ghostCells.GetSize()); ++cellSlot) {
            if (ghostCells[static_cast<std::size_t>(cellSlot)] != nullptr) {
                if (Spark::TransformComponent* tr =
                            ghostCells[static_cast<std::size_t>(cellSlot)]->GetComponent<Spark::TransformComponent>()) {
                    tr->SetTranslation({-20.0F, -20.0F, 0.0F});
                }
            }
        }
    }
}  // namespace Spark
