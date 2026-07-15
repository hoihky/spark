#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/BlendModeComponent.hpp"
#include "spark/ecs/components/SpriteComponent.hpp"
#include "spark/ecs/components/TextOverlayComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/render/SceneBlendMode.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace Spark {

namespace Detail {

[[nodiscard]] inline Spark::SharedPtr<Spark::Texture2D> MakeGemAtlas() {
    constexpr std::uint32_t tw = 22;
    constexpr std::uint32_t au = 3;
    constexpr std::uint32_t av = 2;
    constexpr std::uint32_t w = au * tw;
    constexpr std::uint32_t h = av * tw;
    Spark::Texture2D tex(Spark::Utf8String("Connect3GemAtlas"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    const Spark::Vector3 palette[6] = {
            {0.95F, 0.32F, 0.38F},
            {0.28F, 0.72F, 0.98F},
            {0.42F, 0.92F, 0.38F},
            {0.95F, 0.78F, 0.22F},
            {0.72F, 0.38F, 0.95F},
            {0.98F, 0.52F, 0.28F},
    };
    for (std::uint32_t ty = 0; ty < av; ++ty) {
        for (std::uint32_t tx = 0; tx < au; ++tx) {
            const std::uint32_t id = ty * au + tx;
            const Spark::Vector3 c = (id < 6U) ? palette[id] : Spark::Vector3{0.15F, 0.16F, 0.18F};
            for (std::uint32_t py = 0; py < tw; ++py) {
                for (std::uint32_t px0 = 0; px0 < tw; ++px0) {
                    const float fx = (static_cast<float>(px0) + 0.5F) / static_cast<float>(tw) - 0.5F;
                    const float fy = (static_cast<float>(py) + 0.5F) / static_cast<float>(tw) - 0.5F;
                    const float r = std::sqrt(fx * fx + fy * fy);
                    Spark::Vector3 out = c;
                    if (r > 0.42F) {
                        out = out * 0.55F;
                    } else if (r > 0.36F) {
                        out = {out.x * 1.08F, out.y * 1.08F, out.z * 1.08F};
                    }
                    const std::uint32_t gx = tx * tw + px0;
                    const std::uint32_t gy = ty * tw + py;
                    const std::size_t di = (static_cast<std::size_t>(gy) * w + gx) * 4U;
                    px[di] = static_cast<std::uint8_t>(std::min(255.0F, out.x * 255.0F));
                    px[di + 1U] = static_cast<std::uint8_t>(std::min(255.0F, out.y * 255.0F));
                    px[di + 2U] = static_cast<std::uint8_t>(std::min(255.0F, out.z * 255.0F));
                    px[di + 3U] = 255;
                }
            }
        }
    }
    tex.SetPixels(w, h, Spark::MoveTemp(px));
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

}  // namespace Detail

/**
 * Match-3 style board (swap adjacent gems, clear runs of 3+ horizontally or vertically, gravity, refill).
 * Uses Camera2D + TilemapComponent + mouse picking (TerrainScreenToWorldRay on z=0).
 *
 * **Grid row y matches tilemap iy** (identity `SetTile`, consistent with SceneSubmit’s +Y per row): **y = 0** is
 * the **top** of the board; **y = kRows - 1** is the **floor**. After clears, gems **fall toward larger y**;
 * new gems fill empty cells **from y = 0 downward** (top of the column).
 */
class Connect3Demo {
public:
    static constexpr int kCols = 8;
    static constexpr int kRows = 8;
    static constexpr int kGemTypes = 6;
    static constexpr float kTileWorld = 1.0F;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    static std::uint32_t timingHackU32(Spark::IEngineContext& context);


    void RandSeed(std::uint32_t s) noexcept;

    [[nodiscard]] std::uint32_t RandU32() noexcept;

    [[nodiscard]] int RandGem() noexcept;


    [[nodiscard]] std::size_t Idx(int x, int y) const noexcept;


    [[nodiscard]] bool PickCell(Spark::IEngineContext& context, float mx, float my, int& outIx, int& outIy) const;


    void FillBoardNoMatches();


    /** True if (x,y) sits in a horizontal or vertical run of >=3 equal non-zero gems (reads current grid). */
    [[nodiscard]] bool CellParticipatesInLineOfThree(int x, int y) const noexcept;


    /** If we wrote gem g at (x,y), would that cell lie in a row or column of >=3 matching gems? */
    [[nodiscard]] bool GemWouldFormLineOfThree(int x, int y, std::uint8_t g) noexcept;


    [[nodiscard]] bool CollectMatches(Spark::Array<std::uint8_t>& outMark) noexcept;


    [[nodiscard]] bool TrySwap(int ax, int ay, int bx, int by) noexcept;


    void ApplyGravityAndFill() noexcept;


    void PushBoardToTilemap();

    void UpdateBlendOverlays();

    [[nodiscard]] static Spark::Vector4 GemTileUv(std::uint16_t tileId) noexcept;


    Spark::Array<Spark::GameObject*> roots{};
    Spark::Camera2D camera{};
    Spark::SharedPtr<Spark::Texture2D> atlasTex{};
    Spark::GameObject* boardGo = nullptr;
    Spark::TilemapComponent* tilemap = nullptr;
    Spark::GameObject* hudGo = nullptr;
    Spark::TextOverlayComponent* hudText = nullptr;
    Spark::GameObject* boardPadGo = nullptr;
    Spark::GameObject* cursorGlowGo = nullptr;
    Spark::GameObject* selectionGlowGo = nullptr;
    Spark::GameObject* matchFlashGo = nullptr;
    float matchFlashT = 0.0F;
    float fpsSmoothed = 0.0F;

    Spark::Array<std::uint8_t> grid{};
    int selX = -1;
    int selY = -1;
    int cursorX = 0;
    int cursorY = 0;
    int lastMoveDx = 1;
    int lastMoveDy = 0;
    int score = 0;
    int moves = 0;
    std::uint32_t rng = 1;

};

}  // namespace Spark
