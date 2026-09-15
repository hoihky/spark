#ifndef SPARK_WATER_DETAIL_GLSL
#define SPARK_WATER_DETAIL_GLSL

const float WATER_DETAIL_LAYER1_SCALE = 0.11;
const float WATER_DETAIL_LAYER2_SCALE = 0.36;
const float WATER_DETAIL_LAYER3_SCALE = 0.95;

float waterDetailLayerHeight(vec2 uv) {
    return sin(uv.x * 1.15 + uv.y * 0.92) * 0.48 + sin(uv.x * 2.05 - uv.y * 1.73) * 0.32 +
           sin(uv.x * 0.62 + uv.y * 2.41) * 0.20;
}

float waterDetailHeight(vec2 worldXZ, float timeSeconds) {
    vec2 scroll1 = vec2(timeSeconds * 0.10, timeSeconds * 0.07);
    vec2 scroll2 = vec2(-timeSeconds * 0.17, timeSeconds * 0.13);
    vec2 scroll3 = vec2(timeSeconds * 0.28, -timeSeconds * 0.21);
    float h1 = waterDetailLayerHeight(worldXZ * WATER_DETAIL_LAYER1_SCALE + scroll1);
    float h2 = waterDetailLayerHeight(worldXZ * WATER_DETAIL_LAYER2_SCALE + scroll2 + vec2(11.3, 4.7));
    float h3 = waterDetailLayerHeight(worldXZ * WATER_DETAIL_LAYER3_SCALE + scroll3 + vec2(-6.1, 19.2));
    return h1 + h2 * 0.58 + h3 * 0.30;
}

/** Three-scale scrolling procedural normal added to the Gerstner macro surface. */
vec3 waterDetailNormal(vec2 worldXZ, float timeSeconds, float strength) {
    const float eps = 0.045;
    float hL = waterDetailHeight(worldXZ - vec2(eps, 0.0), timeSeconds);
    float hR = waterDetailHeight(worldXZ + vec2(eps, 0.0), timeSeconds);
    float hD = waterDetailHeight(worldXZ - vec2(0.0, eps), timeSeconds);
    float hU = waterDetailHeight(worldXZ + vec2(0.0, eps), timeSeconds);
    vec3 detailN = normalize(vec3(hL - hR, 2.0 * eps, hD - hU));
    return mix(vec3(0.0, 1.0, 0.0), detailN, clamp(strength, 0.0, 1.0));
}

vec3 waterCombineNormals(vec3 macroNormal, vec3 detailNormal, float detailStrength) {
    float s = clamp(detailStrength, 0.0, 1.0);
    return normalize(vec3(
            macroNormal.x + detailNormal.x * s,
            macroNormal.y * detailNormal.y,
            macroNormal.z + detailNormal.z * s));
}

#endif
