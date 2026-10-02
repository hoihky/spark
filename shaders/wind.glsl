#ifndef SPARK_WIND_GLSL
#define SPARK_WIND_GLSL

#include "scene_ubo.glsl"

vec3 sparkWindDirectionXZ() {
    vec3 dir = vec3(ubo.windDirectionSpeed.x, 0.0, ubo.windDirectionSpeed.z);
    float len2 = dot(dir.xz, dir.xz);
    if (len2 < 1e-6) {
        return vec3(1.0, 0.0, 0.0);
    }
    return dir * inversesqrt(len2);
}

float sparkWindStrength(float instancePhase) {
    float speed = ubo.windDirectionSpeed.w;
    if (speed <= 0.0) {
        return 0.0;
    }
    float t = ubo.windGustParams.w;
    float gust = ubo.windGustParams.x * sin(t * ubo.windGustParams.y * 6.28318 + instancePhase);
    float turbulence =
            ubo.windGustParams.z * 0.22 * sin(dot(vec2(instancePhase, t), vec2(1.7, 2.3)) + t * 1.9);
    return clamp(speed + gust + turbulence, 0.0, 4.0);
}

/** Bend grass in local space; base stays near y = 0. Displacement is capped for tall blades. */
vec3 sparkWindBendLocalPosition(vec3 localPos, float bladeHeight, float bendScale, float instancePhase) {
    float height01 = clamp(localPos.y / max(bladeHeight, 1e-4), 0.0, 1.0);
    if (height01 <= 0.0) {
        return localPos;
    }
    float strength = sparkWindStrength(instancePhase);
    if (strength <= 0.0) {
        return localPos;
    }

    vec3 windDir = sparkWindDirectionXZ();
    vec3 side = normalize(vec3(-windDir.z, 0.0, windDir.x));

    float leanMeters = bendScale * min(strength * 0.14, 0.55) * height01 * height01;
    float swayMeters = sin(ubo.windGustParams.w * 3.1 + instancePhase) * bendScale * 0.11 * height01;

    localPos.x += windDir.x * leanMeters + side.x * swayMeters;
    localPos.z += windDir.z * leanMeters + side.z * swayMeters;
    localPos.y -= leanMeters * 0.08;
    return localPos;
}

#endif
