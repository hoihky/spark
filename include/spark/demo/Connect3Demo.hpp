#pragma once

#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/audio/SoundEngine.hpp"
#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace Spark {

namespace Detail {

[[nodiscard]] inline float GemFacetShade(const float fx, const float fy, const float r) noexcept {
    const float facetA = std::fabs(fx * 0.92F + fy * 0.38F);
    const float facetB = std::fabs(fx * -0.35F + fy * 0.94F);
    const float facet = 0.55F + 0.22F * std::sin(facetA * 14.0F) + 0.18F * std::cos(facetB * 11.0F);
    const float rim = std::clamp((0.48F - r) / 0.10F, 0.0F, 1.0F);
    return std::clamp(facet * (0.80F + 0.20F * rim), 0.35F, 1.35F);
}

[[nodiscard]] inline Spark::SharedPtr<Spark::Texture2D> MakeGemAtlas() {
    constexpr std::uint32_t tw = 64;
    constexpr std::uint32_t au = 3;
    constexpr std::uint32_t av = 2;
    constexpr std::uint32_t w = au * tw;
    constexpr std::uint32_t h = av * tw;
    Spark::Texture2D tex(Spark::Utf8String("Connect3GemAtlas"));
    Spark::Array<std::uint8_t> px;
    px.Resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    const Spark::Vector3 palette[6] = {
            {0.98F, 0.28F, 0.42F},
            {0.22F, 0.68F, 1.0F},
            {0.34F, 0.95F, 0.42F},
            {1.0F, 0.82F, 0.18F},
            {0.78F, 0.34F, 0.98F},
            {1.0F, 0.48F, 0.22F},
    };
    const Spark::Vector3 deep[6] = {
            {0.42F, 0.05F, 0.12F},
            {0.04F, 0.18F, 0.42F},
            {0.05F, 0.28F, 0.08F},
            {0.38F, 0.24F, 0.02F},
            {0.22F, 0.05F, 0.38F},
            {0.42F, 0.12F, 0.03F},
    };
    for (std::uint32_t ty = 0; ty < av; ++ty) {
        for (std::uint32_t tx = 0; tx < au; ++tx) {
            const std::uint32_t id = ty * au + tx;
            const Spark::Vector3 c = (id < 6U) ? palette[id] : Spark::Vector3{0.15F, 0.16F, 0.18F};
            const Spark::Vector3 d = (id < 6U) ? deep[id] : Spark::Vector3{0.05F, 0.05F, 0.06F};
            for (std::uint32_t py = 0; py < tw; ++py) {
                for (std::uint32_t px0 = 0; px0 < tw; ++px0) {
                    const float fx = (static_cast<float>(px0) + 0.5F) / static_cast<float>(tw) - 0.5F;
                    const float fy = (static_cast<float>(py) + 0.5F) / static_cast<float>(tw) - 0.5F;
                    const float r = std::sqrt(fx * fx + fy * fy);
                    constexpr float kGemRadius = 0.475F;
                    constexpr float kEdgeSoft = 0.014F;
                    Spark::Vector3 out = d;
                    float alpha = 0.0F;
                    if (r <= kGemRadius + kEdgeSoft) {
                        alpha = 1.0F;
                        if (r > kGemRadius - kEdgeSoft) {
                            alpha = std::clamp((kGemRadius + kEdgeSoft - r) / (2.0F * kEdgeSoft), 0.0F, 1.0F);
                        }
                        const float shade = GemFacetShade(fx, fy, r);
                        out = {d.x + (c.x - d.x) * shade, d.y + (c.y - d.y) * shade, d.z + (c.z - d.z) * shade};
                        if (r > kGemRadius * 0.76F) {
                            const float rimT =
                                    std::clamp((r - kGemRadius * 0.76F) / (kGemRadius * 0.24F), 0.0F, 1.0F);
                            out = out * (1.0F - rimT * 0.18F) + Spark::Vector3{1.0F, 1.0F, 1.0F} * (rimT * 0.18F);
                        }
                        const float hx = fx + 0.14F;
                        const float hy = fy - 0.16F;
                        const float spec = std::exp(-(hx * hx + hy * hy) * 95.0F);
                        out = out + Spark::Vector3{spec * 0.85F, spec * 0.9F, spec};
                        const float hx2 = fx - 0.1F;
                        const float hy2 = fy + 0.08F;
                        const float spec2 = std::exp(-(hx2 * hx2 + hy2 * hy2) * 120.0F);
                        out = out + Spark::Vector3{spec2 * 0.35F, spec2 * 0.42F, spec2 * 0.55F};
                    }
                    const std::uint32_t gx = tx * tw + px0;
                    const std::uint32_t gy = ty * tw + py;
                    const std::size_t di = (static_cast<std::size_t>(gy) * w + gx) * 4U;
                    px[di] = static_cast<std::uint8_t>(std::min(255.0F, out.x * 255.0F));
                    px[di + 1U] = static_cast<std::uint8_t>(std::min(255.0F, out.y * 255.0F));
                    px[di + 2U] = static_cast<std::uint8_t>(std::min(255.0F, out.z * 255.0F));
                    px[di + 3U] = static_cast<std::uint8_t>(std::min(255.0F, alpha * 255.0F));
                }
            }
        }
    }
    tex.SetPixels(w, h, Spark::MoveTemp(px));
    tex.SetSceneUploadNearest(true);
    return Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(tex));
}

[[nodiscard]] inline Spark::Vector4 GemTintForType(int gemType) noexcept {
    switch (gemType) {
    case 1:
        return {1.0F, 0.35F, 0.45F, 1.0F};
    case 2:
        return {0.35F, 0.78F, 1.0F, 1.0F};
    case 3:
        return {0.45F, 1.0F, 0.45F, 1.0F};
    case 4:
        return {1.0F, 0.88F, 0.25F, 1.0F};
    case 5:
        return {0.82F, 0.42F, 1.0F, 1.0F};
    case 6:
        return {1.0F, 0.55F, 0.22F, 1.0F};
    default:
        return {1.0F, 1.0F, 1.0F, 1.0F};
    }
}

}  // namespace Detail

/**
 * Match-3 style board (swap adjacent gems, clear runs of 3+ horizontally or vertically, gravity, refill).
 * Uses Camera2D + TilemapComponent + mouse picking (TerrainScreenToWorldRay on z=0).
 */
class Connect3Demo {
public:
    static constexpr int kCols = 8;
    static constexpr int kRows = 8;
    static constexpr int kGemTypes = 6;
    static constexpr float kTileWorld = 1.0F;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Unload(Spark::GameWorld& w);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);
    void Render(Spark::Scene& /*scene*/, Spark::GameWorld& world, Spark::IEngineContext& context);

private:
    static std::uint32_t timingHackU32(Spark::IEngineContext& context);
    void RandSeed(std::uint32_t s) noexcept;
    [[nodiscard]] std::uint32_t RandU32() noexcept;
    [[nodiscard]] int RandGem() noexcept;
    [[nodiscard]] std::size_t Idx(int x, int y) const noexcept;
    [[nodiscard]] bool PickCell(Spark::IEngineContext& context, float mx, float my, int& outIx, int& outIy) const;
    void FillBoardNoMatches();
    [[nodiscard]] bool CellParticipatesInLineOfThree(int x, int y) const noexcept;
    [[nodiscard]] bool GemWouldFormLineOfThree(int x, int y, std::uint8_t g) noexcept;
    [[nodiscard]] bool CollectMatches(Spark::Array<std::uint8_t>& outMark) noexcept;
    [[nodiscard]] int CountMarkedCells(const Spark::Array<std::uint8_t>& mark) const noexcept;
    [[nodiscard]] int MaxMatchRunLength(const Spark::Array<std::uint8_t>& mark) const noexcept;
    void ComputeMatchCenter(const Spark::Array<std::uint8_t>& mark, float& outX, float& outY) const noexcept;
    [[nodiscard]] bool TrySwap(int ax, int ay, int bx, int by, Spark::IEngineContext& context, Spark::GameWorld& world) noexcept;
    void ApplyGravityAndFill() noexcept;
    void PushBoardToTilemap();
    void UpdateBlendOverlays(float animTime) noexcept;
    [[nodiscard]] static Spark::Vector3 GridCellCenter(int x, int y) noexcept;
    [[nodiscard]] static Spark::Vector4 GemTileUv(std::uint16_t tileId) noexcept;
    void SpawnClearVfx(Spark::GameWorld& world, int x, int y, int gemType, int comboLevel, int runLength) const noexcept;
    void SpawnSwapVfx(Spark::GameWorld& world, int ax, int ay, int bx, int by) const noexcept;
    void SpawnInvalidSwapVfx(Spark::GameWorld& world, int ax, int ay, int bx, int by) const noexcept;
    void SpawnComboBurstVfx(Spark::GameWorld& world, float centerX, float centerY, int comboLevel, int cleared) const noexcept;

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
    float matchPadPulseT = 0.0F;
    float invalidShakeT = 0.0F;
    float animTime = 0.0F;
    float cameraShakeT = 0.0F;
    float cameraShakeMag = 0.0F;
    int lastCombo = 0;
    int bestCombo = 0;

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
    Spark::SoundEngine* audioEngine = nullptr;
};

}  // namespace Spark
