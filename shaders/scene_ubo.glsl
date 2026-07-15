#ifndef SPARK_SCENE_UBO_GLSL
#define SPARK_SCENE_UBO_GLSL

layout(std140, set = 0, binding = 0) uniform SceneUBO {
    mat4 viewProj;
    vec4 lightDir;
    vec4 cameraPos;
    vec4 lightColor;
    vec4 ambientColor;
    vec4 ambientSky;
    vec4 ambientProbe;
    mat4 invViewProj;
    vec4 viewportSize;
    vec4 timeGlobal;
    mat4 worldToShadowClip[4];
    vec4 cascadeSplits;
    vec4 cascadeAtlas[4];
    vec4 shadowParams;
    vec4 clusterGrid;
    vec4 clusterDepth;
    /** x = env equirect layer (-1 = procedural sky); y = intensity; w = enabled (1). */
    vec4 iblParams;
} ubo;

#endif
