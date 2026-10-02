#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTex;
layout(location = 3) flat in int vTextureLayer;
layout(location = 4) in vec4 vTint;
layout(location = 5) in float vHeight01;

layout(set = 0, binding = 1) uniform sampler2DArray sceneTextures;

#include "scene_ubo.glsl"
#include "foliage_instance.glsl"

layout(location = 0) out vec4 outColor;

float sparkFoliageInterleavedGradientNoise(vec2 screenPx) {
    return fract(52.9829189 * fract(dot(screenPx, vec2(0.06711056, 0.00583715))));
}

void main() {
    vec3 albedo = vTint.rgb;
    float alpha = 1.0;
    if (vTextureLayer >= 0) {
        vec4 tex = texture(sceneTextures, vec3(vTex, float(vTextureLayer)));
        albedo *= tex.rgb;
        alpha = tex.a;
    } else {
        albedo *= vec3(0.22, 0.62, 0.18);
        alpha = vHeight01;
    }

    alpha *= smoothstep(0.0, 0.18, vTex.y);
    float baseDarken = mix(0.5, 1.05, vHeight01);
    albedo *= baseDarken;

    float viewDist = length(vWorldPos.xz - ubo.cameraPos.xz);
    float fadeStart = foliageBatch.fadeStartDistance;
    float fadeEnd = foliageBatch.fadeEndDistance;
    if (fadeEnd > fadeStart + 0.25) {
        float viewFade = 1.0 - smoothstep(fadeStart, fadeEnd, viewDist);
        float dither = sparkFoliageInterleavedGradientNoise(gl_FragCoord.xy);
        if (viewFade < dither) {
            discard;
        }
        alpha *= viewFade;
    }

    if (alpha < foliageBatch.alphaCutoff) {
        discard;
    }

    vec3 n = normalize(vNormal);
    vec3 lightDir = normalize(-ubo.lightDir.xyz);
    float ndl = abs(dot(n, lightDir));
    float wrap = max(dot(n, lightDir), 0.0) * 0.58 + 0.42;
    float backScatter = max(dot(n, -lightDir), 0.0) * 0.14;
    vec3 viewDir = normalize(ubo.cameraPos.xyz - vWorldPos);
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 2.2) * 0.2;

    vec3 sun = ubo.lightColor.rgb * ubo.lightColor.w * (ndl * 0.72 + wrap * 0.32 + backScatter);
    vec3 ambient = ubo.ambientColor.rgb * 1.25;
    vec3 color = albedo * (ambient + sun) + rim * vec3(0.48, 0.68, 0.38);
    outColor = vec4(color, 1.0);
}
