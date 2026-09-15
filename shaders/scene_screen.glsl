#ifndef SPARK_SCENE_SCREEN_GLSL
#define SPARK_SCENE_SCREEN_GLSL

#include "scene_ubo.glsl"

vec2 sparkSceneScreenUv(vec2 clipNdc) {
    vec2 uv = vec2(clipNdc.x * 0.5 + 0.5, (1.0 - clipNdc.y) * 0.5);
    if (ubo.viewportSize.w > 0.5) {
        uv.y = 1.0 - uv.y;
    }
    return uv;
}

vec2 sparkSceneScreenUvFromWorld(vec3 worldPos) {
    vec4 clip = ubo.viewProj * vec4(worldPos, 1.0);
    if (abs(clip.w) < 1e-5) {
        vec2 vp = max(ubo.viewportSize.xy, vec2(1.0));
        return gl_FragCoord.xy / vp;
    }
    return sparkSceneScreenUv(clip.xy / clip.w);
}

vec2 sparkSceneScreenUvToNdc(vec2 uv) {
    vec2 ndc = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    if (ubo.viewportSize.w > 0.5) {
        ndc.y = uv.y * 2.0 - 1.0;
    }
    return ndc;
}

/** Reconstruct world position from screen UV and a stored depth sample (binding 14). */
vec3 sparkSceneReconstructWorld(vec2 uv, float depthSample) {
    vec2 ndc = sparkSceneScreenUvToNdc(uv);
    vec4 clip = vec4(ndc, depthSample, 1.0);
    vec4 world = ubo.invViewProj * clip;
    return world.xyz / max(world.w, 1.0e-5);
}

#endif
