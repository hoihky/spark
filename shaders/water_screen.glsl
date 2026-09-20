#ifndef SPARK_WATER_SCREEN_GLSL
#define SPARK_WATER_SCREEN_GLSL

#include "scene_ubo.glsl"

/**
 * Water-only screen UV (Vulkan framebuffer / gl_FragCoord convention):
 *   y = 0 at TOP, y = 1 at BOTTOM — matches copied bindings 13/14 with texture() and no V flip.
 *
 * post_common uses the opposite Y (y = 1 top) plus a MoltenVK flip at sample time.
 * Do not mix the two in water shaders.
 */
vec2 waterFramebufferScreenUv() {
    vec2 fullSize = max(ubo.viewportSize.xy, vec2(1.0));
    return (gl_FragCoord.xy - vec2(0.5)) / fullSize;
}

vec2 waterNdcToScreenUv(vec2 ndc) {
    return ndc * 0.5 + 0.5;
}

vec2 waterScreenUvToNdc(vec2 uv) {
    return uv * 2.0 - 1.0;
}

vec2 waterWorldToScreenUv(vec3 worldPos) {
    vec4 clip = ubo.viewProj * vec4(worldPos, 1.0);
    if (abs(clip.w) < 1e-5) {
        return waterFramebufferScreenUv();
    }
    return waterNdcToScreenUv(clip.xy / clip.w);
}

vec2 waterOpaqueTextureUv(vec2 screenUv) {
    return clamp(screenUv, vec2(0.001), vec2(0.999));
}

ivec2 waterOpaquePixel(vec2 screenUv) {
    vec2 fullSize = max(ubo.viewportSize.xy, vec2(1.0));
    vec2 uv = clamp(screenUv, vec2(0.0), vec2(1.0));
    ivec2 pix = ivec2(uv * fullSize - vec2(0.5));
    return clamp(pix, ivec2(0), ivec2(fullSize) - ivec2(1));
}

vec3 waterReconstructWorld(vec2 screenUv, float depthSample) {
    vec2 ndc = waterScreenUvToNdc(screenUv);
    vec4 clip = vec4(ndc, depthSample, 1.0);
    vec4 world = ubo.invViewProj * clip;
    return world.xyz / max(world.w, 1.0e-5);
}

#endif
