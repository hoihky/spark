// Scene texture array is stored as UNORM; albedo/sky art is sRGB-encoded in files.
#ifndef SPARK_COLOR_SPACE_GLSL
#define SPARK_COLOR_SPACE_GLSL

vec3 sparkSrgbToLinear(vec3 srgb) {
    vec3 lo = srgb / 12.92;
    vec3 hi = pow((srgb + vec3(0.055)) / vec3(1.055), vec3(2.4));
    return mix(lo, hi, step(vec3(0.04045), srgb));
}

#endif
