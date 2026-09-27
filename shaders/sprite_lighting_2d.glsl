#ifndef SPARK_SPRITE_LIGHTING_2D_GLSL
#define SPARK_SPRITE_LIGHTING_2D_GLSL

#include "clustered_lights.glsl"

vec3 sparkSpriteDecodeNormal(vec2 normalUv, int normalLayer, float strength) {
    if (normalLayer < 0) {
        return vec3(0.0, 0.0, 1.0);
    }
    vec3 nmap = textureLod(spriteSceneTextures, vec3(normalUv, float(normalLayer)), 0.0).rgb;
    vec2 nxy = (nmap.xy * 2.0 - 1.0) * max(0.0, strength);
    float nz2 = max(0.0025, 1.0 - dot(nxy, nxy));
    return normalize(vec3(nxy, sqrt(nz2)));
}

vec3 sparkSpriteAccumDirectional(vec3 albedo, vec3 n, float ambScale, float dirScale) {
    vec3 ldir = normalize(vec3(ubo.lightDir.xy, 0.0));
    float ndl = max(0.0, dot(n, ldir));
    vec3 amb = ubo.ambientColor.rgb * clamp(ambScale, 0.0, 1.0);
    vec3 sun = ubo.lightColor.rgb * ubo.lightColor.w * ndl * max(0.0, dirScale);
    return albedo * (amb + sun);
}

vec3 sparkSpriteAccumPointLights(vec3 albedo, vec3 n, vec3 worldPos, float difScale) {
    vec3 acc = vec3(0.0);
    if (ubo.clusterDepth.w < 0.5) {
        return acc;
    }
    uint numPt = min(clusterLights.numPointLights, kMaxClusteredPointLights);
    for (uint i = 0u; i < numPt; ++i) {
        vec3 p = clusterLights.pointPositionRange[i].xyz;
        float range = max(clusterLights.pointPositionRange[i].w, 1e-3);
        vec3 lc = clusterLights.pointColorIntensity[i].xyz;
        float intens = clusterLights.pointColorIntensity[i].w;
        vec3 toL = p - worldPos;
        float d = length(toL);
        float att = sparkPointAttenuation(d, range);
        float ndl = max(0.0, dot(n, normalize(toL)));
        acc += albedo * lc * intens * att * ndl * max(0.0, difScale);
    }
    return acc;
}

#endif
