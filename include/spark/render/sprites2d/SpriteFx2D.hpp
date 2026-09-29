#pragma once

#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/render/sprites2d/SpriteLighting2D.hpp"

#include <algorithm>

namespace Spark {

/** Helpers for parametric sprite FX (uses <c>SpriteLighting2DComponent</c> modes 13–15). */
namespace SpriteFx2D {

/** @p sceneTimeSeconds is written to param1.y (see <c>sprite.frag</c> mode 13). */
inline void ApplyHitFlashAtSceneTime(
        SpriteLighting2DComponent& lighting,
        const Vector4& flashColor,
        const float decaySeconds,
        const float sceneTimeSeconds) noexcept {
    lighting.SetMode(SpriteLighting2DMode::HitFlash);
    lighting.SetParam0(flashColor);
    lighting.SetParam1({std::max(0.01F, decaySeconds), sceneTimeSeconds, 0.0F, 0.0F});
}

inline void ApplyHitFlash(SpriteLighting2DComponent& lighting, const Vector4& flashColor, const float decaySeconds) noexcept {
    ApplyHitFlashAtSceneTime(lighting, flashColor, decaySeconds, 0.0F);
}

inline void ApplyOutline(
        SpriteLighting2DComponent& lighting,
        const Vector4& outlineColor,
        const float widthTexels,
        const float softness) noexcept {
    lighting.SetMode(SpriteLighting2DMode::Outline);
    lighting.SetParam0({outlineColor.x, outlineColor.y, outlineColor.z, widthTexels});
    lighting.SetParam1({softness, 0.0F, 0.0F, 0.0F});
}

inline void ApplyDissolve(
        SpriteLighting2DComponent& lighting,
        const float edge,
        const float noiseScale,
        const float softness,
        const float scrollSpeed) noexcept {
    lighting.SetMode(SpriteLighting2DMode::Dissolve);
    lighting.SetParam0({edge, noiseScale, softness, scrollSpeed});
}

/** @p progress01 0 = visible, 1 = fully dissolved (see <c>sprite.frag</c> mode 15). */
inline void ApplyDissolveProgress(
        SpriteLighting2DComponent& lighting,
        const float progress01,
        const float noiseScale = 8.0F,
        const float softness = 0.08F,
        const float scrollSpeed = 1.5F) noexcept {
    const float edge = std::clamp(progress01, 0.0F, 1.0F);
    ApplyDissolve(lighting, edge, noiseScale, softness, scrollSpeed);
}

}  // namespace SpriteFx2D

}  // namespace Spark
