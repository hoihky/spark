#pragma once

#include "spark/ecs/components/rendering/SpriteLighting2DComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/render/sprites2d/SpriteLighting2D.hpp"

#include <algorithm>

namespace Spark {

/** Helpers for parametric sprite FX (uses <c>SpriteLighting2DComponent</c> modes 13–15). */
namespace SpriteFx2D {

inline void ApplyHitFlash(SpriteLighting2DComponent& lighting, const Vector4& flashColor, const float decaySeconds) noexcept {
    lighting.SetMode(SpriteLighting2DMode::HitFlash);
    lighting.SetParam0(flashColor);
    lighting.SetParam1({std::max(0.01F, decaySeconds), 0.0F, 0.0F, 0.0F});
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

}  // namespace SpriteFx2D

}  // namespace Spark
