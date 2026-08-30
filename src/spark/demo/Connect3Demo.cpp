#include "spark/demo/Connect3Demo.hpp"

#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/scene/vfx/VfxSubsystemProcess.hpp"

#include <cmath>

namespace Spark {

void Connect3Demo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        atlasTex = Detail::MakeGemAtlas();
        w.RegisterTexture(atlasTex, "spark/connect3/gems");

        boardGo = w.CreateGameObject();
        boardGo->GetName() = Utf8String("Connect3Board");
        boardGo->AddComponent<TransformComponent>();
        tilemap = boardGo->AddComponent<TilemapComponent>(
                atlasTex,
                static_cast<std::uint32_t>(kCols),
                static_cast<std::uint32_t>(kRows),
                3,
                2,
                kTileWorld,
                12);
        roots.PushBack(boardGo);

        boardPadGo = w.CreateGameObject();
        boardPadGo->GetName() = Utf8String("Connect3BoardPad");
        {
            TransformComponent* tr = boardPadGo->AddComponent<TransformComponent>();
            tr->SetTranslation({static_cast<float>(kCols) * 0.5F * kTileWorld, static_cast<float>(kRows) * 0.5F * kTileWorld, -0.02F});
            tr->SetScale({static_cast<float>(kCols) * kTileWorld + 0.4F, static_cast<float>(kRows) * kTileWorld + 0.4F, 1.0F});
        }
        boardPadGo->AddComponent<BlendModeComponent>(SceneBlendMode::Multiply);
        boardPadGo->AddComponent<SpriteComponent>(
                atlasTex,
                Vector4{0.14F, 0.16F, 0.24F, 0.92F},
                Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                4);
        roots.PushBack(boardPadGo);

        cursorGlowGo = w.CreateGameObject();
        cursorGlowGo->GetName() = Utf8String("Connect3CursorGlow");
        {
            TransformComponent* tr = cursorGlowGo->AddComponent<TransformComponent>();
            tr->SetUniformScale(kTileWorld * 1.1F);
        }
        cursorGlowGo->AddComponent<BlendModeComponent>(SceneBlendMode::Screen);
        cursorGlowGo->AddComponent<SpriteComponent>(
                atlasTex,
                Vector4{0.45F, 0.88F, 1.0F, 0.62F},
                GemTileUv(0),
                20);
        roots.PushBack(cursorGlowGo);

        selectionGlowGo = w.CreateGameObject();
        selectionGlowGo->GetName() = Utf8String("Connect3SelectionGlow");
        {
            TransformComponent* tr = selectionGlowGo->AddComponent<TransformComponent>();
            tr->SetUniformScale(kTileWorld * 1.14F);
            tr->SetTranslation({-20.0F, -20.0F, 0.0F});
        }
        selectionGlowGo->AddComponent<BlendModeComponent>(SceneBlendMode::Screen);
        selectionGlowGo->AddComponent<SpriteComponent>(
                atlasTex,
                Vector4{1.0F, 0.95F, 0.42F, 0.72F},
                GemTileUv(1),
                21);
        roots.PushBack(selectionGlowGo);

        matchFlashGo = w.CreateGameObject();
        matchFlashGo->GetName() = Utf8String("Connect3MatchFlash");
        {
            TransformComponent* tr = matchFlashGo->AddComponent<TransformComponent>();
            tr->SetUniformScale(kTileWorld * 2.6F);
            tr->SetTranslation({-20.0F, -20.0F, 0.0F});
        }
        matchFlashGo->AddComponent<BlendModeComponent>(SceneBlendMode::Additive);
        matchFlashGo->AddComponent<SpriteComponent>(
                atlasTex,
                Vector4{1.0F, 0.82F, 0.45F, 0.0F},
                GemTileUv(3),
                950);
        roots.PushBack(matchFlashGo);

        hudGo = w.CreateGameObject();
        hudGo->GetName() = Utf8String("Connect3Hud");
        hudText = hudGo->AddComponent<TextOverlayComponent>();
        hudText->SetScreenPosition(DemoHud::kScreenMargin, DemoHud::kScreenMargin);
        DemoHud::Apply(*hudText, false);
        roots.PushBack(hudGo);

        camera.position = {static_cast<float>(kCols) * 0.5F * kTileWorld, static_cast<float>(kRows) * 0.5F * kTileWorld, 0.0F};
        camera.halfExtentY = static_cast<float>(kRows) * 0.58F * kTileWorld;
        camera.rotationRad = 0.0F;

        RandSeed(timingHackU32(context));
        selX = -1;
        selY = -1;
        lastMoveDx = 1;
        lastMoveDy = 0;
        cursorX = kCols / 2;
        cursorY = kRows / 2;
        score = 0;
        moves = 0;
        lastCombo = 0;
        bestCombo = 0;
        animTime = 0.0F;
        FillBoardNoMatches();
        PushBoardToTilemap();
        context.GetInput().SetCursorCaptured(false);
    }

void Connect3Demo::Unload(Spark::GameWorld& w)
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
        cursorGlowGo = nullptr;
        selectionGlowGo = nullptr;
        matchFlashGo = nullptr;
        hudGo = nullptr;
        hudText = nullptr;
    }

void Connect3Demo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world)
{
        IInput& in = context.GetInput();
        const float dt = timing.deltaTimeSeconds;
        animTime += dt;

        if (matchFlashT > 0.0F) {
            matchFlashT = std::max(0.0F, matchFlashT - dt);
            if (matchFlashGo != nullptr) {
                if (SpriteComponent* flash = matchFlashGo->GetComponent<SpriteComponent>()) {
                    const float pulse = matchFlashT / 0.42F;
                    flash->SetTint({1.0F, 0.82F, 0.45F, 0.92F * pulse * pulse});
                }
            }
        }
        if (matchPadPulseT > 0.0F) {
            matchPadPulseT = std::max(0.0F, matchPadPulseT - dt);
        }
        if (invalidShakeT > 0.0F) {
            invalidShakeT = std::max(0.0F, invalidShakeT - dt);
        }
        if (cameraShakeT > 0.0F) {
            cameraShakeT = std::max(0.0F, cameraShakeT - dt);
        }

        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);

        if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
            score = 0;
            moves = 0;
            lastCombo = 0;
            bestCombo = 0;
            selX = -1;
            selY = -1;
            FillBoardNoMatches();
            PushBoardToTilemap();
        }

        if (in.IsKeyPressedThisFrame(GLFW_KEY_LEFT)) {
            lastMoveDx = -1;
            lastMoveDy = 0;
            cursorX = std::max(0, cursorX - 1);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_RIGHT)) {
            lastMoveDx = 1;
            lastMoveDy = 0;
            cursorX = std::min(kCols - 1, cursorX + 1);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_UP)) {
            lastMoveDx = 0;
            lastMoveDy = -1;
            cursorY = std::max(0, cursorY - 1);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_DOWN)) {
            lastMoveDx = 0;
            lastMoveDy = 1;
            cursorY = std::min(kRows - 1, cursorY + 1);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_SPACE)) {
            const int nx = cursorX + lastMoveDx;
            const int ny = cursorY + lastMoveDy;
            if (nx >= 0 && nx < kCols && ny >= 0 && ny < kRows) {
                static_cast<void>(TrySwap(cursorX, cursorY, nx, ny, context, world));
            }
        }

        if (in.IsMouseButtonPressedThisFrame(GLFW_MOUSE_BUTTON_LEFT)) {
            float mx = 0.0F;
            float my = 0.0F;
            in.GetCursorFramebufferPixels(mx, my, fbW, fbH);
            int ix = 0;
            int iy = 0;
            if (PickCell(context, mx, my, ix, iy)) {
                if (selX < 0 || selY < 0) {
                    selX = ix;
                    selY = iy;
                } else if (selX == ix && selY == iy) {
                    selX = -1;
                    selY = -1;
                } else if (std::abs(selX - ix) + std::abs(selY - iy) == 1) {
                    static_cast<void>(TrySwap(selX, selY, ix, iy, context, world));
                    selX = -1;
                    selY = -1;
                } else {
                    selX = ix;
                    selY = iy;
                }
            }
        }

        PushBoardToTilemap();
        UpdateBlendOverlays(animTime);

        if (hudText != nullptr) {
            const std::string selTxt = (selX >= 0) ? std::format("{},{}", selX, selY) : std::string("none");
            const std::string comboTxt = lastCombo > 1 ? std::format(" · combo x{}", lastCombo) : std::string{};
            hudText->SetText(Utf8String(
                    std::format(
                            "score {} · moves {} · best combo {}{} — cursor ({},{}) · sel ({}) · R reset",
                            score,
                            moves,
                            bestCombo,
                            comboTxt,
                            cursorX,
                            cursorY,
                            selTxt)
                            .c_str()));
        }

        ProcessVfx(world);
    }

void Connect3Demo::Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context)
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
        Vector3 camPos = camera.position;
        if (cameraShakeT > 0.0F) {
            const float n = cameraShakeT / 0.14F;
            const float wobble = n * n;
            camPos.x += std::sin(cameraShakeT * 84.0F) * cameraShakeMag * wobble;
            camPos.y += std::cos(cameraShakeT * 71.0F) * cameraShakeMag * wobble;
        }
        if (invalidShakeT > 0.0F) {
            const float n = invalidShakeT / 0.12F;
            camPos.x += std::sin(invalidShakeT * 120.0F) * 0.03F * n;
        }
        Camera2D renderCam = camera;
        renderCam.position = camPos;
        const Matrix4 viewProj = renderCam.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
        Vector3 pr{};
        Vector3 pu{};
        renderCam.BillboardBasisWorld(pr, pu);
        SubmitStandardLitSceneFromWorld(
                world,
                context,
                viewProj,
                camPos,
                Vector3{0.38F, 0.86F, 0.44F}.Normalized(),
                Vector3{0.95F, 0.97F, 1.0F},
                0.78F,
                Vector3{0.06F, 0.07F, 0.1F},
                true,
                pr,
                pu,
                animTime);
    }

std::uint32_t Connect3Demo::timingHackU32(Spark::IEngineContext& context)
{
        int w = 0;
        int h = 0;
        context.GetFramebufferSize(w, h);
        return static_cast<std::uint32_t>(w * 7919 + h * 65537 + 3);
    }

void Connect3Demo::RandSeed(const std::uint32_t s) noexcept
{
        rng = s != 0 ? s : 1U;
}

std::uint32_t Connect3Demo::RandU32() noexcept
{
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }

int Connect3Demo::RandGem() noexcept
{
        return 1 + static_cast<int>(RandU32() % static_cast<std::uint32_t>(kGemTypes));
    }

std::size_t Connect3Demo::Idx(const int x, const int y) const noexcept
{
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(kCols) + static_cast<std::size_t>(x);
    }

bool Connect3Demo::PickCell(Spark::IEngineContext& context, float mx, float my, int& outIx, int& outIy) const
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        if (fbW <= 0 || fbH <= 0) {
            return false;
        }
        const Matrix4 vp = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
        Matrix4 invVp{};
        if (!vp.TryInvert(invVp)) {
            return false;
        }
        Vector3 ro{};
        Vector3 rd{};
        if (!TerrainScreenToWorldRay(fbW, fbH, mx, my, invVp, ro, rd)) {
            return false;
        }
        if (std::fabs(rd.z) < 1.0e-5F) {
            return false;
        }
        const float t = -ro.z / rd.z;
        const float wx = ro.x + rd.x * t;
        const float wy = ro.y + rd.y * t;
        const int ix = static_cast<int>(std::floor(wx / kTileWorld));
        const int iy = static_cast<int>(std::floor(wy / kTileWorld));
        if (ix < 0 || ix >= kCols || iy < 0 || iy >= kRows) {
            return false;
        }
        outIx = ix;
        outIy = iy;
        return true;
    }

void Connect3Demo::FillBoardNoMatches()
{
        grid.Clear();
        grid.Resize(static_cast<std::size_t>(kCols) * static_cast<std::size_t>(kRows));
        for (int y = 0; y < kRows; ++y) {
            for (int x = 0; x < kCols; ++x) {
                int g = RandGem();
                int guard = 0;
                while (GemWouldFormLineOfThree(x, y, static_cast<std::uint8_t>(g)) && guard++ < 80) {
                    g = RandGem();
                }
                grid[Idx(x, y)] = static_cast<std::uint8_t>(g);
            }
        }
    }

bool Connect3Demo::CellParticipatesInLineOfThree(const int x, const int y) const noexcept
{
        const std::uint8_t c = grid[Idx(x, y)];
        if (c == 0) {
            return false;
        }
        int horiz = 1;
        for (int lx = x - 1; lx >= 0 && grid[Idx(lx, y)] == c; --lx) {
            ++horiz;
        }
        for (int rx = x + 1; rx < kCols && grid[Idx(rx, y)] == c; ++rx) {
            ++horiz;
        }
        if (horiz >= 3) {
            return true;
        }
        int vert = 1;
        for (int uy = y - 1; uy >= 0 && grid[Idx(x, uy)] == c; --uy) {
            ++vert;
        }
        for (int dy = y + 1; dy < kRows && grid[Idx(x, dy)] == c; ++dy) {
            ++vert;
        }
        return vert >= 3;
    }

bool Connect3Demo::GemWouldFormLineOfThree(const int x, const int y, const std::uint8_t g) noexcept
{
        const std::size_t i = Idx(x, y);
        const std::uint8_t prev = grid[i];
        grid[i] = g;
        const bool hit = CellParticipatesInLineOfThree(x, y);
        grid[i] = prev;
        return hit;
    }

bool Connect3Demo::CollectMatches(Array<std::uint8_t>& outMark) noexcept
{
        for (std::size_t i = 0; i < outMark.GetSize(); ++i) {
            outMark[i] = 0;
        }
        bool any = false;
        for (int y = 0; y < kRows; ++y) {
            int x = 0;
            while (x < kCols) {
                const std::uint8_t c = grid[Idx(x, y)];
                if (c == 0) {
                    ++x;
                    continue;
                }
                int len = 1;
                while (x + len < kCols && grid[Idx(x + len, y)] == c) {
                    ++len;
                }
                if (len >= 3) {
                    any = true;
                    for (int k = 0; k < len; ++k) {
                        outMark[Idx(x + k, y)] = 1;
                    }
                }
                x += len;
            }
        }
        for (int x = 0; x < kCols; ++x) {
            int y = 0;
            while (y < kRows) {
                const std::uint8_t c = grid[Idx(x, y)];
                if (c == 0) {
                    ++y;
                    continue;
                }
                int len = 1;
                while (y + len < kRows && grid[Idx(x, y + len)] == c) {
                    ++len;
                }
                if (len >= 3) {
                    any = true;
                    for (int k = 0; k < len; ++k) {
                        outMark[Idx(x, y + k)] = 1;
                    }
                }
                y += len;
            }
        }
        return any;
    }

int Connect3Demo::CountMarkedCells(const Array<std::uint8_t>& mark) const noexcept
{
        int count = 0;
        for (std::size_t i = 0; i < mark.GetSize(); ++i) {
            if (mark[i] != 0) {
                ++count;
            }
        }
        return count;
    }

int Connect3Demo::MaxMatchRunLength(const Array<std::uint8_t>& mark) const noexcept
{
        int best = 0;
        for (int y = 0; y < kRows; ++y) {
            int x = 0;
            while (x < kCols) {
                if (mark[Idx(x, y)] == 0) {
                    ++x;
                    continue;
                }
                int len = 1;
                const std::uint8_t c = grid[Idx(x, y)];
                while (x + len < kCols && mark[Idx(x + len, y)] != 0 && grid[Idx(x + len, y)] == c) {
                    ++len;
                }
                best = std::max(best, len);
                x += len;
            }
        }
        for (int x = 0; x < kCols; ++x) {
            int y = 0;
            while (y < kRows) {
                if (mark[Idx(x, y)] == 0) {
                    ++y;
                    continue;
                }
                int len = 1;
                const std::uint8_t c = grid[Idx(x, y)];
                while (y + len < kRows && mark[Idx(x, y + len)] != 0 && grid[Idx(x, y + len)] == c) {
                    ++len;
                }
                best = std::max(best, len);
                y += len;
            }
        }
        return best;
    }

void Connect3Demo::ComputeMatchCenter(const Array<std::uint8_t>& mark, float& outX, float& outY) const noexcept
{
        float sx = 0.0F;
        float sy = 0.0F;
        int count = 0;
        for (int y = 0; y < kRows; ++y) {
            for (int x = 0; x < kCols; ++x) {
                if (mark[Idx(x, y)] == 0) {
                    continue;
                }
                const Vector3 c = GridCellCenter(x, y);
                sx += c.x;
                sy += c.y;
                ++count;
            }
        }
        if (count > 0) {
            outX = sx / static_cast<float>(count);
            outY = sy / static_cast<float>(count);
        } else {
            outX = static_cast<float>(kCols) * 0.5F * kTileWorld;
            outY = static_cast<float>(kRows) * 0.5F * kTileWorld;
        }
    }

Vector3 Connect3Demo::GridCellCenter(const int x, const int y) noexcept
{
        return {
                (static_cast<float>(x) + 0.5F) * kTileWorld,
                (static_cast<float>(y) + 0.5F) * kTileWorld,
                0.08F};
}

void Connect3Demo::SpawnClearVfx(
        GameWorld& world,
        const int x,
        const int y,
        const int gemType,
        const int comboLevel,
        const int runLength) const noexcept
{
        const Vector3 pos = GridCellCenter(x, y);
        world.GetVfxSubsystem().Queue("vfx/connect3_clear", pos);
        world.GetVfxSubsystem().Queue("impact", pos);
        if (runLength >= 5) {
            world.GetVfxSubsystem().Queue("level_up", pos);
        } else if (runLength >= 4) {
            world.GetVfxSubsystem().Queue("laser_hit", pos);
        }
        if (comboLevel >= 3) {
            world.GetVfxSubsystem().Queue("shockwave", {pos.x, pos.y, 0.1F});
        }
        (void)gemType;
}

void Connect3Demo::SpawnSwapVfx(GameWorld& world, const int ax, const int ay, const int bx, const int by) const noexcept
{
        world.GetVfxSubsystem().Queue("vfx/connect3_clear", GridCellCenter(ax, ay));
        world.GetVfxSubsystem().Queue("vfx/connect3_clear", GridCellCenter(bx, by));
}

void Connect3Demo::SpawnInvalidSwapVfx(GameWorld& world, const int ax, const int ay, const int bx, const int by) const noexcept
{
        const Vector3 mid{
                (GridCellCenter(ax, ay).x + GridCellCenter(bx, by).x) * 0.5F,
                (GridCellCenter(ax, ay).y + GridCellCenter(bx, by).y) * 0.5F,
                0.07F};
        world.GetVfxSubsystem().Queue("dust", mid);
}

void Connect3Demo::SpawnComboBurstVfx(
        GameWorld& world,
        const float centerX,
        const float centerY,
        const int comboLevel,
        const int cleared) const noexcept
{
        if (comboLevel >= 2) {
            world.GetVfxSubsystem().Queue("level_up", {centerX, centerY, 0.11F});
        }
        if (comboLevel >= 3 || cleared >= 6) {
            world.GetVfxSubsystem().Queue("shockwave", {centerX, centerY, 0.12F});
        }
        if (comboLevel >= 4 || cleared >= 9) {
            world.GetVfxSubsystem().Queue("confetti", {centerX, centerY, 0.14F});
        }
}

bool Connect3Demo::TrySwap(const int ax, const int ay, const int bx, const int by, IEngineContext& context, GameWorld& world) noexcept
{
        if (std::abs(ax - bx) + std::abs(ay - by) != 1) {
            return false;
        }
        if (grid[Idx(ax, ay)] == 0 || grid[Idx(bx, by)] == 0) {
            return false;
        }
        std::swap(grid[Idx(ax, ay)], grid[Idx(bx, by)]);
        Array<std::uint8_t> mark;
        mark.Resize(static_cast<std::size_t>(kCols) * static_cast<std::size_t>(kRows));
        if (!CollectMatches(mark)) {
            std::swap(grid[Idx(ax, ay)], grid[Idx(bx, by)]);
            invalidShakeT = 0.12F;
            SpawnInvalidSwapVfx(world, ax, ay, bx, by);
            DemoPlayProceduralClip(context, DemoSfx::ClipMatch3Invalid(), 0.85F);
            return false;
        }
        if (mark[Idx(ax, ay)] == 0 && mark[Idx(bx, by)] == 0) {
            std::swap(grid[Idx(ax, ay)], grid[Idx(bx, by)]);
            invalidShakeT = 0.12F;
            SpawnInvalidSwapVfx(world, ax, ay, bx, by);
            DemoPlayProceduralClip(context, DemoSfx::ClipMatch3Invalid(), 0.85F);
            return false;
        }

        ++moves;
        lastCombo = 0;
        SpawnSwapVfx(world, ax, ay, bx, by);
        DemoPlayProceduralClip(context, DemoSfx::ClipMatch3Swap(), 0.9F);

        matchFlashT = 0.42F;
        matchPadPulseT = 0.22F;
        if (matchFlashGo != nullptr) {
            if (TransformComponent* tr = matchFlashGo->GetComponent<TransformComponent>()) {
                const float mx = (static_cast<float>(ax + bx) + 1.0F) * 0.5F * kTileWorld;
                const float my = (static_cast<float>(ay + by) + 1.0F) * 0.5F * kTileWorld;
                tr->SetTranslation({mx, my, 0.04F});
            }
            if (SpriteComponent* flash = matchFlashGo->GetComponent<SpriteComponent>()) {
                const int gemType = static_cast<int>(grid[Idx(ax, ay)]);
                const Vector4 tint = Detail::GemTintForType(gemType);
                flash->SetTint({tint.x, tint.y, tint.z, 0.95F});
            }
        }

        int cascadeGuard = 0;
        int comboLevel = 0;
        do {
            const int runLength = MaxMatchRunLength(mark);
            const int cleared = CountMarkedCells(mark);
            float centerX = 0.0F;
            float centerY = 0.0F;
            ComputeMatchCenter(mark, centerX, centerY);

            ++comboLevel;
            lastCombo = comboLevel;
            bestCombo = std::max(bestCombo, comboLevel);

            for (int y = 0; y < kRows; ++y) {
                for (int x = 0; x < kCols; ++x) {
                    if (mark[Idx(x, y)] == 0) {
                        continue;
                    }
                    const int gemType = static_cast<int>(grid[Idx(x, y)]);
                    SpawnClearVfx(world, x, y, gemType, comboLevel, runLength);
                    grid[Idx(x, y)] = 0;
                }
            }

            const int multiplier = std::max(1, comboLevel);
            score += cleared * (10 + runLength * 2) * multiplier;
            SpawnComboBurstVfx(world, centerX, centerY, comboLevel, cleared);

            if (comboLevel == 1) {
                DemoPlayProceduralClip(context, DemoSfx::ClipMatch3Clear(), 0.95F);
            } else {
                DemoPlayProceduralClip(context, DemoSfx::ClipMatch3Combo(), 0.85F + static_cast<float>(comboLevel) * 0.04F);
            }
            if (comboLevel >= 2) {
                cameraShakeT = 0.1F + static_cast<float>(comboLevel) * 0.02F;
                cameraShakeMag = 0.03F + static_cast<float>(comboLevel) * 0.012F;
            }
            if (runLength >= 5) {
                cameraShakeMag += 0.03F;
            }

            ApplyGravityAndFill();
            ++cascadeGuard;
        } while (CollectMatches(mark) && cascadeGuard < 64);

        return true;
    }

void Connect3Demo::ApplyGravityAndFill() noexcept
{
        for (int x = 0; x < kCols; ++x) {
            Array<std::uint8_t> col;
            for (int y = kRows - 1; y >= 0; --y) {
                const std::uint8_t v = grid[Idx(x, y)];
                if (v != 0) {
                    col.PushBack(v);
                }
            }
            int dest = kRows - 1;
            for (std::size_t i = 0; i < col.GetSize(); ++i) {
                grid[Idx(x, dest)] = col[i];
                --dest;
            }
            for (int y = 0; y <= dest; ++y) {
                grid[Idx(x, y)] = 0;
            }
            for (int y = 0; y <= dest; ++y) {
                int g = RandGem();
                int guard = 0;
                while (GemWouldFormLineOfThree(x, y, static_cast<std::uint8_t>(g)) && guard++ < 80) {
                    g = RandGem();
                }
                grid[Idx(x, y)] = static_cast<std::uint8_t>(g);
            }
        }
    }

void Connect3Demo::PushBoardToTilemap()
{
        if (tilemap == nullptr) {
            return;
        }
        for (int y = 0; y < kRows; ++y) {
            for (int x = 0; x < kCols; ++x) {
                const std::uint8_t g = grid[Idx(x, y)];
                const std::uint32_t tileIy = static_cast<std::uint32_t>(y);
                if (g == 0) {
                    tilemap->SetTile(static_cast<std::uint32_t>(x), tileIy, TilemapComponent::kEmptyTile);
                } else {
                    const std::uint16_t tid = static_cast<std::uint16_t>(static_cast<unsigned>(g) - 1U);
                    tilemap->SetTile(static_cast<std::uint32_t>(x), tileIy, tid);
                }
            }
        }
    }

Vector4 Connect3Demo::GemTileUv(const std::uint16_t tileId) noexcept
{
        constexpr std::uint32_t kAtlasU = 3U;
        constexpr std::uint32_t kAtlasV = 2U;
        const std::uint32_t id = static_cast<std::uint32_t>(tileId);
        const std::uint32_t tx = id % kAtlasU;
        const std::uint32_t ty = id / kAtlasU;
        const float du = 1.0F / static_cast<float>(kAtlasU);
        const float dv = 1.0F / static_cast<float>(kAtlasV);
        return {static_cast<float>(tx) * du, static_cast<float>(ty) * dv, static_cast<float>(tx + 1U) * du,
                static_cast<float>(ty + 1U) * dv};
    }

void Connect3Demo::UpdateBlendOverlays(const float animTimeVal) noexcept
{
        const float pulse = 0.88F + 0.12F * std::sin(animTimeVal * 5.2F);
        const float selPulse = 0.9F + 0.1F * std::sin(animTimeVal * 7.5F + 1.2F);

        if (cursorGlowGo != nullptr) {
            if (TransformComponent* tr = cursorGlowGo->GetComponent<TransformComponent>()) {
                tr->SetTranslation(
                        {(static_cast<float>(cursorX) + 0.5F) * kTileWorld,
                         (static_cast<float>(cursorY) + 0.5F) * kTileWorld,
                         0.01F});
                tr->SetUniformScale(kTileWorld * 1.1F * pulse);
            }
            if (SpriteComponent* spr = cursorGlowGo->GetComponent<SpriteComponent>()) {
                spr->SetTint({0.45F, 0.88F, 1.0F, 0.45F + 0.2F * pulse});
            }
        }
        if (selectionGlowGo != nullptr) {
            if (TransformComponent* tr = selectionGlowGo->GetComponent<TransformComponent>()) {
                if (selX >= 0 && selY >= 0) {
                    tr->SetTranslation(
                            {(static_cast<float>(selX) + 0.5F) * kTileWorld,
                             (static_cast<float>(selY) + 0.5F) * kTileWorld,
                             0.02F});
                    tr->SetUniformScale(kTileWorld * 1.14F * selPulse);
                    if (SpriteComponent* spr = selectionGlowGo->GetComponent<SpriteComponent>()) {
                        const int gemType = static_cast<int>(grid[Idx(selX, selY)]);
                        const Vector4 tint = Detail::GemTintForType(gemType);
                        spr->SetTint({tint.x, tint.y, tint.z, 0.55F + 0.25F * selPulse});
                        spr->SetUvRect(GemTileUv(static_cast<std::uint16_t>(gemType > 0 ? gemType - 1 : 0)));
                    }
                } else {
                    tr->SetTranslation({-20.0F, -20.0F, 0.0F});
                }
            }
        }
        if (boardPadGo != nullptr) {
            if (TransformComponent* tr = boardPadGo->GetComponent<TransformComponent>()) {
                const float padW = static_cast<float>(kCols) * kTileWorld + 0.4F;
                const float padH = static_cast<float>(kRows) * kTileWorld + 0.4F;
                float scaleBoost = 1.0F;
                if (matchPadPulseT > 0.0F) {
                    const float t = matchPadPulseT / 0.22F;
                    scaleBoost = 1.0F + 0.022F * std::sin(t * 3.14159265F);
                }
                tr->SetScale({padW * scaleBoost, padH * scaleBoost, 1.0F});
            }
            if (SpriteComponent* pad = boardPadGo->GetComponent<SpriteComponent>()) {
                const float brighten = matchPadPulseT > 0.0F ? 0.05F * (matchPadPulseT / 0.22F) : 0.0F;
                pad->SetTint({0.14F + brighten, 0.16F + brighten, 0.24F + brighten, 0.92F});
            }
        }
    }

}  // namespace Spark
