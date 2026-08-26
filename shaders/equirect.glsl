#ifndef SPARK_EQUIRECT_GLSL
#define SPARK_EQUIRECT_GLSL

const float SPARK_EQUIRECT_PI = 3.14159265359;

// Scene layers letterbox non-square uploads into the bottom-left of a square array layer.
// layerUvScale.xy is the occupied fraction (from Texture2D::GetSceneLayerUvScale()).
vec2 sparkEquirectDirectionUv(vec3 dir, vec2 layerUvScale) {
    float phi = atan(dir.z, dir.x) / (2.0 * SPARK_EQUIRECT_PI) + 0.5;
    phi = fract(phi) * layerUvScale.x;
    float mu = acos(clamp(dir.y, -1.0, 1.0)) / SPARK_EQUIRECT_PI;
    float vEq = (1.0 - mu) * layerUvScale.y;
    return vec2(phi, vEq);
}

#endif
