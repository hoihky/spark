#ifndef SPARK_SPRITE_INSTANCE_GLSL
#define SPARK_SPRITE_INSTANCE_GLSL

struct SpriteInstanceGpu {
    mat4 model;
    vec4 tint;
    vec4 uvRect;
    vec4 normalUvRect;
    int textureLayer;
    int lightingMode;
    int normalTextureLayer;
    int rampTextureLayer;
    vec4 lightingA;
    vec4 lightingB;
};

layout(std430, set = 0, binding = 9) readonly buffer SpriteInstanceBuffer {
    SpriteInstanceGpu instances[];
};

layout(push_constant) uniform SpriteBatch {
    uint instanceBase;
} spriteBatch;

#endif
