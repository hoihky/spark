#include "spark/demo/Connect3Demo.hpp"

#include "spark/ecs/components/BlendModeComponent.hpp"
#include "spark/ecs/components/SpriteComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/render/SceneBlendMode.hpp"

namespace Spark {

void Connect3Demo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        atlasTex = Detail::MakeGemAtlas();
        w.RegisterTexture(atlasTex, "spark/connect3/gems");

        boardGo = w.CreateGameObject();
        boardGo->GetName() = Spark::Utf8String("Connect3Board");
        boardGo->AddComponent<Spark::TransformComponent>();
        tilemap = boardGo->AddComponent<Spark::TilemapComponent>(
                atlasTex,
                static_cast<std::uint32_t>(kCols),
                static_cast<std::uint32_t>(kRows),
                3,
                2,
                kTileWorld,
                12);
        roots.PushBack(boardGo);

        boardPadGo = w.CreateGameObject();
        boardPadGo->GetName() = Spark::Utf8String("Connect3BoardPad");
        {
            Spark::TransformComponent* tr = boardPadGo->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({static_cast<float>(kCols) * 0.5F * kTileWorld, static_cast<float>(kRows) * 0.5F * kTileWorld, -0.02F});
            tr->SetScale({static_cast<float>(kCols) * kTileWorld + 0.4F, static_cast<float>(kRows) * kTileWorld + 0.4F, 1.0F});
        }
        boardPadGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Multiply);
        boardPadGo->AddComponent<Spark::SpriteComponent>(
                atlasTex,
                Spark::Vector4{0.18F, 0.20F, 0.28F, 0.9F},
                Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                4);
        roots.PushBack(boardPadGo);

        cursorGlowGo = w.CreateGameObject();
        cursorGlowGo->GetName() = Spark::Utf8String("Connect3CursorGlow");
        {
            Spark::TransformComponent* tr = cursorGlowGo->AddComponent<Spark::TransformComponent>();
            tr->SetUniformScale(kTileWorld * 1.08F);
        }
        cursorGlowGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Screen);
        cursorGlowGo->AddComponent<Spark::SpriteComponent>(
                atlasTex,
                Spark::Vector4{0.55F, 0.82F, 1.0F, 0.55F},
                GemTileUv(0),
                20);
        roots.PushBack(cursorGlowGo);

        selectionGlowGo = w.CreateGameObject();
        selectionGlowGo->GetName() = Spark::Utf8String("Connect3SelectionGlow");
        {
            Spark::TransformComponent* tr = selectionGlowGo->AddComponent<Spark::TransformComponent>();
            tr->SetUniformScale(kTileWorld * 1.12F);
            tr->SetTranslation({-20.0F, -20.0F, 0.0F});
        }
        selectionGlowGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Screen);
        selectionGlowGo->AddComponent<Spark::SpriteComponent>(
                atlasTex,
                Spark::Vector4{1.0F, 0.92F, 0.35F, 0.62F},
                GemTileUv(1),
                21);
        roots.PushBack(selectionGlowGo);

        matchFlashGo = w.CreateGameObject();
        matchFlashGo->GetName() = Spark::Utf8String("Connect3MatchFlash");
        {
            Spark::TransformComponent* tr = matchFlashGo->AddComponent<Spark::TransformComponent>();
            tr->SetUniformScale(kTileWorld * 2.4F);
            tr->SetTranslation({-20.0F, -20.0F, 0.0F});
        }
        matchFlashGo->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
        matchFlashGo->AddComponent<Spark::SpriteComponent>(
                atlasTex,
                Spark::Vector4{1.0F, 0.75F, 0.35F, 0.0F},
                GemTileUv(3),
                950);
        roots.PushBack(matchFlashGo);

        hudGo = w.CreateGameObject();
        hudGo->GetName() = Spark::Utf8String("Connect3Hud");
        hudText = hudGo->AddComponent<Spark::TextOverlayComponent>();
        hudText->SetScreenPosition(12.0F, 12.0F);
        hudText->SetFontSizePixels(19.0F);
        hudText->SetColor({0.96F, 0.98F, 1.0F});
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

void Connect3Demo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        Spark::IInput& in = context.GetInput();
        const float dt = timing.deltaTimeSeconds;
        if (matchFlashT > 0.0F) {
            matchFlashT = std::max(0.0F, matchFlashT - dt);
            if (matchFlashGo != nullptr) {
                if (Spark::SpriteComponent* flash = matchFlashGo->GetComponent<Spark::SpriteComponent>()) {
                    const float pulse = matchFlashT / 0.38F;
                    flash->SetTint({1.0F, 0.75F, 0.35F, 0.9F * pulse});
                }
            }
        }

        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);

        if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
            score = 0;
            moves = 0;
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
                static_cast<void>(TrySwap(cursorX, cursorY, nx, ny));
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
                    static_cast<void>(TrySwap(selX, selY, ix, iy));
                    selX = -1;
                    selY = -1;
                } else {
                    selX = ix;
                    selY = iy;
                }
            }
        }

        PushBoardToTilemap();
        UpdateBlendOverlays();

        if (hudText != nullptr) {
            const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            const std::string selTxt = (selX >= 0) ? std::format("{},{}", selX, selY) : std::string("none");
            hudText->SetText(Spark::Utf8String(
                    std::format(
                            "Match-3 — {:.0f} FPS — score {} · moves {} — screen cursor/sel · additive match flash · LMB swap · R reshuffle\n"
                            "cursor ({},{}) · mouse sel ({})",
                            static_cast<double>(fpsSmoothed),
                            score,
                            moves,
                            cursorX,
                            cursorY,
                            selTxt)
                            .c_str()));
        }
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
                Spark::Vector3{0.38F, 0.86F, 0.44F}.Normalized(),
                Spark::Vector3{0.95F, 0.97F, 1.0F},
                0.72F,
                Spark::Vector3{0.07F, 0.08F, 0.11F},
                false,
                pr,
                pu,
                0.0F);
    }

std::uint32_t Connect3Demo::timingHackU32(Spark::IEngineContext& context)
{
        int w = 0;
        int h = 0;
        context.GetFramebufferSize(w, h);
        return static_cast<std::uint32_t>(w * 7919 + h * 65537 + 3);
    }

void Connect3Demo::RandSeed(std::uint32_t s) noexcept
{ rng = s != 0 ? s : 1U; }

[[nodiscard]] std::uint32_t Connect3Demo::RandU32() noexcept
{
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }

[[nodiscard]] int Connect3Demo::RandGem() noexcept
{ return 1 + static_cast<int>(RandU32() % static_cast<std::uint32_t>(kGemTypes)); }

[[nodiscard]] std::size_t Connect3Demo::Idx(int x, int y) const noexcept
{
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(kCols) + static_cast<std::size_t>(x);
    }

[[nodiscard]] bool Connect3Demo::PickCell(Spark::IEngineContext& context, float mx, float my, int& outIx, int& outIy) const
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        if (fbW <= 0 || fbH <= 0) {
            return false;
        }
        const Spark::Matrix4 vp = camera.ViewProjection(static_cast<float>(fbW), static_cast<float>(fbH));
        Spark::Matrix4 invVp{};
        if (!vp.TryInvert(invVp)) {
            return false;
        }
        Spark::Vector3 ro{};
        Spark::Vector3 rd{};
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

/** True if (x,y) sits in a horizontal or vertical run of >=3 equal non-zero gems (reads current grid). */
    [[nodiscard]] bool Connect3Demo::CellParticipatesInLineOfThree(int x, int y) const noexcept
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

/** If we wrote gem g at (x,y), would that cell lie in a row or column of >=3 matching gems? */
    [[nodiscard]] bool Connect3Demo::GemWouldFormLineOfThree(int x, int y, std::uint8_t g) noexcept
{
        const std::size_t i = Idx(x, y);
        const std::uint8_t prev = grid[i];
        grid[i] = g;
        const bool hit = CellParticipatesInLineOfThree(x, y);
        grid[i] = prev;
        return hit;
    }

[[nodiscard]] bool Connect3Demo::CollectMatches(Spark::Array<std::uint8_t>& outMark) noexcept
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

[[nodiscard]] bool Connect3Demo::TrySwap(int ax, int ay, int bx, int by) noexcept
{
        if (std::abs(ax - bx) + std::abs(ay - by) != 1) {
            return false;
        }
        if (grid[Idx(ax, ay)] == 0 || grid[Idx(bx, by)] == 0) {
            return false;
        }
        std::swap(grid[Idx(ax, ay)], grid[Idx(bx, by)]);
        Spark::Array<std::uint8_t> mark;
        mark.Resize(static_cast<std::size_t>(kCols) * static_cast<std::size_t>(kRows));
        if (!CollectMatches(mark)) {
            std::swap(grid[Idx(ax, ay)], grid[Idx(bx, by)]);
            return false;
        }
        if (mark[Idx(ax, ay)] == 0 && mark[Idx(bx, by)] == 0) {
            std::swap(grid[Idx(ax, ay)], grid[Idx(bx, by)]);
            return false;
        }
        ++moves;
        matchFlashT = 0.38F;
        if (matchFlashGo != nullptr) {
            if (Spark::TransformComponent* tr = matchFlashGo->GetComponent<Spark::TransformComponent>()) {
                const float mx = (static_cast<float>(ax + bx) + 1.0F) * 0.5F * kTileWorld;
                const float my = (static_cast<float>(ay + by) + 1.0F) * 0.5F * kTileWorld;
                tr->SetTranslation({mx, my, 0.04F});
            }
        }
        int cascadeGuard = 0;
        do {
            int cleared = 0;
            for (int y = 0; y < kRows; ++y) {
                for (int x = 0; x < kCols; ++x) {
                    if (mark[Idx(x, y)] != 0) {
                        grid[Idx(x, y)] = 0;
                        ++cleared;
                    }
                }
            }
            score += cleared * 10;
            ApplyGravityAndFill();
            ++cascadeGuard;
        } while (CollectMatches(mark) && cascadeGuard < 64);
        return true;
    }

void Connect3Demo::ApplyGravityAndFill() noexcept
{
        for (int x = 0; x < kCols; ++x) {
            Spark::Array<std::uint8_t> col;
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
                    tilemap->SetTile(static_cast<std::uint32_t>(x), tileIy, Spark::TilemapComponent::kEmptyTile);
                } else {
                    const std::uint16_t tid = static_cast<std::uint16_t>(static_cast<unsigned>(g)-1U);
                    tilemap->SetTile(static_cast<std::uint32_t>(x), tileIy, tid);
                }
            }
        }
    }

Spark::Vector4 Connect3Demo::GemTileUv(const std::uint16_t tileId) noexcept
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

void Connect3Demo::UpdateBlendOverlays()
{
        if (cursorGlowGo != nullptr) {
            if (Spark::TransformComponent* tr = cursorGlowGo->GetComponent<Spark::TransformComponent>()) {
                tr->SetTranslation(
                        {(static_cast<float>(cursorX) + 0.5F) * kTileWorld,
                         (static_cast<float>(cursorY) + 0.5F) * kTileWorld,
                         0.01F});
            }
        }
        if (selectionGlowGo != nullptr) {
            if (Spark::TransformComponent* tr = selectionGlowGo->GetComponent<Spark::TransformComponent>()) {
                if (selX >= 0 && selY >= 0) {
                    tr->SetTranslation(
                            {(static_cast<float>(selX) + 0.5F) * kTileWorld,
                             (static_cast<float>(selY) + 0.5F) * kTileWorld,
                             0.02F});
                } else {
                    tr->SetTranslation({-20.0F, -20.0F, 0.0F});
                }
            }
        }
    }
}  // namespace Spark
