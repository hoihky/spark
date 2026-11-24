#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

struct SceneRenderParams;

/** Style for ephemeral floating numbers (damage, heals, pickups). */
struct FloatingCombatTextStyle {
    float fontSizePixels = 20.0F;
    Vector3 color{1.0F, 0.25F, 0.2F};
    float alphaStart = 1.0F;
    float lifetimeSeconds = 0.85F;
    /** World +Y drift before projection each frame (screen-space path). */
    float riseWorldUnitsPerSecond = 1.4F;
    /** When true, draws use <c>screenOverlayTexts</c> (above base HUD, below late UI). */
    bool useOverlayLayer = true;
};

/**
 * Screen-projected floating combat text — recommended for damage numbers in 2D/3D action games.
 *
 * Not a <c>UiCanvasComponent</c> (screen retained UI). Spawns into <c>SceneRenderParams</c> after world submit.
 * For digit-atlas motion tied to world depth sort, use pooled <c>SpriteComponent</c> quads instead (see guide).
 */
class FloatingCombatTextPresenter final {
public:
    static constexpr std::size_t kMaxActive = 64;

    void Clear() noexcept;
    void SpawnAtWorld(const Vector3& worldAnchor, Utf8String text, const FloatingCombatTextStyle* styleOverride = nullptr);
    void SpawnDamage(const Vector3& worldAnchor, float appliedAmount, const FloatingCombatTextStyle* styleOverride = nullptr);
    void Advance(float deltaTimeSeconds) noexcept;
    void AppendDraws(SceneRenderParams& params, float framebufferWidth, float framebufferHeight) const noexcept;

    [[nodiscard]] std::size_t ActiveCount() const noexcept { return entries.GetSize(); }

private:
    struct Entry {
        Vector3 worldAnchor{};
        Utf8String text{};
        float ageSeconds = 0.0F;
        float lifetimeSeconds = 0.85F;
        float riseWorldUnitsPerSecond = 1.4F;
        float fontSizePixels = 20.0F;
        Vector3 color{1.0F, 0.25F, 0.2F};
        float alphaStart = 1.0F;
        bool useOverlayLayer = true;
    };

    void PushEntry(Entry entry);

    Array<Entry> entries{};
    FloatingCombatTextStyle defaultStyle{};
};

}  // namespace Spark