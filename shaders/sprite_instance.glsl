#ifndef SPARK_SPRITE_INSTANCE_GLSL
#define SPARK_SPRITE_INSTANCE_GLSL

struct SpriteInstanceGpu {
    mat4 model;
    vec4 tint;
    vec4 uvRect;
    int textureLayer;
    int lightingMode;
    float lightingPad0;
    float lightingPad1;
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
