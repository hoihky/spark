#ifndef SPARK_SCENE_PUSH_GLSL
#define SPARK_SCENE_PUSH_GLSL

layout(push_constant) uniform Push {
    mat4 model;
    vec4 albedoTint;
    int textureLayer;
    int skyMode;
    float metallic;
    float roughness;
    vec4 emissive;
    int useSkinning;
    int jointCount;
    int shadingModel;
    int toonDiffuseBands;
    float toonRimIntensity;
    float toonRimPower;
    int normalMapLayer;
    int metallicRoughnessMapLayer;
    int emissiveMapLayer;
    float metallicFactor;
    float roughnessFactor;
    float occlusionStrength;
    int shadowFlags;
    float alphaCutoff;
    vec2 mapUvScale[4];
    vec2 mapUvOffset[4];
    float mapUvRotation[4];
    int mapTexCoordSet[4];
    vec4 emissiveFactor;
    int albedoHdrLinear;
    int pushPad;
} push;

#endif
