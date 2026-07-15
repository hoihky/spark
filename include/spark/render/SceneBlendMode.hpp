#pragma once

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Compositing mode for 2D draws (sprites, UI rects, screen text).
 * Maps to fixed-function blend state via <c>VulkanBlendAttachmentForMode</c>.
 *
 * Typical usage:
 * - <c>AlphaOver</c> — default sprites and UI.
 * - <c>PremultipliedAlpha</c> — exported UI / VFX atlases.
 * - <c>Multiply</c> — ground shadows, water tint, ambient occlusion overlays.
 * - <c>Screen</c> — soft light / glow overlays.
 * - <c>Additive</c> — sparks, magic, water highlights.
 * - <c>Opaque</c> — solid UI fills (no blending).
 */
enum class SceneBlendMode : std::uint8_t {
    Opaque = 0,
    AlphaOver = 1,
    PremultipliedAlpha = 2,
    Multiply = 3,
    Screen = 4,
    Additive = 5,
};

inline constexpr std::size_t kSceneBlendModeCount = 6U;

/** Default when no <c>BlendModeComponent</c> is present on a drawable. */
inline constexpr SceneBlendMode kSceneBlendModeDefault = SceneBlendMode::AlphaOver;

/**
 * Pass draw order (lower = submitted and rasterized earlier).
 * Multiply/shadow layers sit under standard alpha, additive on top.
 */
[[nodiscard]] constexpr std::uint8_t GetSceneBlendModePassOrder(const SceneBlendMode mode) noexcept {
    switch (mode) {
    case SceneBlendMode::Opaque:
        return 1U;
    case SceneBlendMode::AlphaOver:
    case SceneBlendMode::PremultipliedAlpha:
        return 2U;
    case SceneBlendMode::Screen:
        return 3U;
    case SceneBlendMode::Additive:
        return 4U;
    case SceneBlendMode::Multiply:
    default:
        return 0U;
    }
}

[[nodiscard]] constexpr bool SceneBlendModeUsesAlphaBlend(const SceneBlendMode mode) noexcept {
    return mode != SceneBlendMode::Opaque;
}

[[nodiscard]] constexpr const char* SceneBlendModeName(const SceneBlendMode mode) noexcept {
    switch (mode) {
    case SceneBlendMode::Opaque:
        return "Opaque";
    case SceneBlendMode::AlphaOver:
        return "AlphaOver";
    case SceneBlendMode::PremultipliedAlpha:
        return "PremultipliedAlpha";
    case SceneBlendMode::Multiply:
        return "Multiply";
    case SceneBlendMode::Screen:
        return "Screen";
    case SceneBlendMode::Additive:
        return "Additive";
    }
    return "AlphaOver";
}

}  // namespace Spark
