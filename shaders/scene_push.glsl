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
    int iridescenceThicknessMapLayer;
    float metallicFactor;
    float roughnessFactor;
    float occlusionStrength;
    int shadowFlags;
    float alphaCutoff;
    vec2 mapUvScale[5];
    vec2 mapUvOffset[5];
    float mapUvRotation[5];
    int mapTexCoordSet[5];
    float clearcoatFactor;
    float clearcoatRoughnessFactor;
    float emissiveStrength;
    float transmissionFactor;
    float emissiveFactor[3];
    int albedoHdrLinear;
    float normalScale;
    float iridescenceFactor;
    float iridescenceIor;
    float iridescenceThicknessMin;
    float iridescenceThicknessMax;
} push;

#endif
