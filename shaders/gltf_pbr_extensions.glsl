// glTF KHR material extensions: clearcoat, transmission, emissive_strength.
#ifndef SPARK_GLTF_PBR_EXTENSIONS_GLSL
#define SPARK_GLTF_PBR_EXTENSIONS_GLSL

layout(set = 0, binding = 13) uniform sampler2D sceneOpaqueColor;

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

vec3 sparkSampleOpaqueBackground(vec3 worldPos, vec3 N, vec3 V) {
    vec3 refractDir = refract(-V, N, 1.0 / 1.5);
    if (dot(refractDir, refractDir) < 1e-6) {
        refractDir = reflect(-V, N);
    } else {
        refractDir = normalize(refractDir);
    }
    const float glassThickness = 0.04;
    vec2 uvDirect = sparkSceneScreenUvFromWorld(worldPos);
    vec2 uvRefract = sparkSceneScreenUvFromWorld(worldPos + refractDir * glassThickness);
    vec2 offset = uvRefract - uvDirect;
    vec2 sampleUv = clamp(uvDirect + offset * 2.5, vec2(0.001), vec2(0.999));
    return texture(sceneOpaqueColor, sampleUv).rgb;
}

vec3 sparkComputeEmissive(vec4 vEmissive, int emissiveMapLayer) {
    float strength = max(push.emissiveStrength, 0.0);
    if (strength < 1e-6) {
        strength = 1.0;
    }
    vec3 e = vEmissive.rgb * vEmissive.w * vec3(push.emissiveFactor[0], push.emissiveFactor[1], push.emissiveFactor[2]) * strength;
    if (emissiveMapLayer >= 0) {
        int el = clamp(emissiveMapLayer, 0, 63);
        e *= texture(sceneTextures, vec3(sparkMapUv(3), float(el))).rgb;
    }
    return e;
}

void sparkAccumulateClearcoat(
        vec3 nGeom,
        vec3 V,
        vec3 Ld,
        vec3 sunRad,
        float sunShadow,
        float occlusion,
        inout vec3 Lo,
        inout vec3 iblSpecular) {
    float cc = push.clearcoatFactor;
    if (cc < 1e-4) {
        return;
    }
    // Clearcoat uses the base geometry normal (not orange-peel bumps) per glTF layering.
    float ccRough = clamp(push.clearcoatRoughnessFactor, 0.06, 1.0);
    Lo += cc * evalBRDF(nGeom, V, Ld, vec3(1.0), 0.0, ccRough, sunRad, 0.0) * sunShadow;
    iblSpecular += cc * sparkEvalSpecularIbl(sceneTextures, sceneHdrTextures, nGeom, V, vec3(1.0), 0.0, ccRough, occlusion);
}

float sparkSampleIridescenceThickness() {
    if (push.iridescenceThicknessMapLayer < 0) {
        return 0.5;
    }
    int tl = clamp(push.iridescenceThicknessMapLayer, 0, 63);
    return texture(sceneTextures, vec3(sparkMapUv(4), float(tl))).r;
}

/** Thin-film hue multiplier for specular channels (1 = no tint). */
vec3 sparkEvalIridescenceSpecTint(vec3 N, vec3 V, vec3 base) {
    float factor = push.iridescenceFactor;
    if (factor < 1e-4) {
        return vec3(1.0);
    }
    float thickness = mix(push.iridescenceThicknessMin, push.iridescenceThicknessMax, sparkSampleIridescenceThickness());
    float nDotV = max(dot(N, V), 0.001);
    float opticalPath = thickness * 2.5e-4 * push.iridescenceIor / max(nDotV, 0.05);
    vec3 hue;
    hue.r = 0.5 + 0.5 * cos(opticalPath * 6.28318);
    hue.g = 0.5 + 0.5 * cos(opticalPath * 6.28318 + 2.094);
    hue.b = 0.5 + 0.5 * cos(opticalPath * 6.28318 + 4.188);
    hue = clamp(hue, 0.0, 1.0);
    float view = pow(1.0 - nDotV, 2.5);
    vec3 f0 = mix(vec3(0.04), base, 0.35);
    float f0Lum = max(dot(f0, vec3(0.2126, 0.7152, 0.0722)), 0.04);
    vec3 tinted = hue * f0Lum;
    return mix(vec3(1.0), tinted / f0Lum, clamp(factor * view, 0.0, 1.0));
}

void sparkApplyTransmission(
        vec3 worldPos,
        vec3 N,
        vec3 V,
        vec3 base,
        float metallic,
        inout vec3 color,
        inout float alpha) {
    float trans = push.transmissionFactor;
    if (trans < 1e-3) {
        return;
    }
    float nDotV = max(dot(N, V), 0.001);
    float fresnel = 0.04 + (1.0 - 0.04) * pow(1.0 - nDotV, 5.0);
    vec3 attenuation = max(base * (1.0 - metallic), vec3(0.02));
    vec3 behind = sparkSampleOpaqueBackground(worldPos, N, V) * attenuation;
    float transMix = trans * (1.0 - fresnel * 0.88);
    color = mix(color, behind, transMix);
    float glassAlpha = mix(0.04, 0.78, fresnel);
    alpha = mix(alpha, glassAlpha, trans);
}

#endif
