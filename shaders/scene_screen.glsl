#ifndef SPARK_SCENE_SCREEN_GLSL
#define SPARK_SCENE_SCREEN_GLSL

#include "scene_ubo.glsl"

/**
 * Engine screen UV convention (post_common, water.vert, clustered lights):
 *   uv.x = ndc.x * 0.5 + 0.5
 *   uv.y = (1.0 - ndc.y) * 0.5   →  y = 1 at TOP, y = 0 at BOTTOM (Vulkan NDC y = -1 top)
 *
 * gl_FragCoord alone is the opposite (y = 0 top). Always convert fragCoord → projection UV
 * before depth reconstruction or pairing with sparkSceneScreenUvFromWorld().
 */
vec2 sparkSceneScreenUv(vec2 clipNdc) {
    return vec2(clipNdc.x * 0.5 + 0.5, (1.0 - clipNdc.y) * 0.5);
}

vec2 sparkSceneScreenUvToNdc(vec2 uv) {
    return vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
}

/** gl_FragCoord → projection screen UV (matches vScreenUv / post_common). */
vec2 sparkSceneFragmentScreenUv() {
    vec2 fullSize = max(ubo.viewportSize.xy, vec2(1.0));
    vec2 fragUv = (gl_FragCoord.xy - vec2(0.5)) / fullSize;
    return vec2(fragUv.x, 1.0 - fragUv.y);
}

vec2 sparkSceneScreenUvFromWorld(vec3 worldPos) {
    vec4 clip = ubo.viewProj * vec4(worldPos, 1.0);
    if (abs(clip.w) < 1e-5) {
        return sparkSceneFragmentScreenUv();
    }
    return sparkSceneScreenUv(clip.xy / clip.w);
}

/** texelFetch coords: projection UV (y=1 top) → Vulkan upper-left pixel (y=0 top). */
ivec2 sparkSceneOpaquePixel(vec2 screenUv) {
    vec2 fullSize = max(ubo.viewportSize.xy, vec2(1.0));
    vec2 uv = clamp(screenUv, vec2(0.0), vec2(1.0));
    ivec2 pix = ivec2(uv.x * fullSize.x - 0.5, (1.0 - uv.y) * fullSize.y - 0.5);
    return clamp(pix, ivec2(0), ivec2(fullSize) - ivec2(1));
}

/**
 * texture() UV for bindings 13/14. Projection UV + MoltenVK row flip (same as post_common
 * depthSampleUv) so color and depth stay paired.
 */
vec2 sparkSceneOpaqueTextureUv(vec2 screenUv) {
    vec2 uv = clamp(screenUv, vec2(0.0), vec2(1.0));
    if (ubo.viewportSize.w > 0.5) {
        uv.y = 1.0 - uv.y;
    }
    return uv;
}

/** Reconstruct world position from projection screen UV and a stored depth sample (binding 14). */
vec3 sparkSceneReconstructWorld(vec2 uv, float depthSample) {
    vec2 ndc = sparkSceneScreenUvToNdc(uv);
    vec4 clip = vec4(ndc, depthSample, 1.0);
    vec4 world = ubo.invViewProj * clip;
    return world.xyz / max(world.w, 1.0e-5);
}

#endif
