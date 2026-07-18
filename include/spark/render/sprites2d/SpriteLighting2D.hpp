#pragma once

#include <cstdint>

namespace Spark {

/**
 * Built-in 2D sprite lighting / shading modes (fragment path). Game code selects a mode on
 * SpriteLighting2DComponent and tunes param0/param1; custom engines can extend the shader + enum together.
 */
enum class SpriteLighting2DMode : std::int32_t {
    /** Default: texture * tint only (same as sprites without SpriteLighting2DComponent). */
    None = 0,
    /**
     * Fake Lambert on the XY quad: radial "normal" from sprite center vs directional light (scene XY).
     * param0.x = ambient scale (0–1), param0.y = diffuse scale; param0.zw unused.
     */
    DirectionalLambert = 1,
    /**
     * Rim / silhouette highlight using view-facing falloff.
     * param0.rgb = rim color, param0.a = power; param1.x = intensity scale.
     */
    Rim = 2,
    /**
     * Pulsing emissive overlay (gemstones, torches). param0.rgb = glow color, param0.w = pulse frequency (Hz),
     * param1.x = emission strength, param1.y = base tint mix (0–1).
     */
    PulseEmission = 3,
    /**
     * Sum scene point lights (first N in UBO) with range attenuation at sprite fragment world position.
     * param0.x = ambient add, param0.y = diffuse scale; param1.xyz unused.
     */
    PointSoft = 4,
};

}  // namespace Spark
