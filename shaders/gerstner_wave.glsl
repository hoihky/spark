#ifndef SPARK_GERSTNER_WAVE_GLSL
#define SPARK_GERSTNER_WAVE_GLSL

const float SPARK_GERSTNER_TWO_PI = 6.28318530718;

struct WaterGerstnerWaveGpu {
    vec2 direction;
    float amplitude;
    float wavelength;
    float speed;
    float steepness;
    vec2 padding_;
};

void sparkGerstnerAccumulate(
        vec2 worldXZ,
        float timeSeconds,
        WaterGerstnerWaveGpu wave,
        inout vec3 displacement,
        inout vec3 tangent,
        inout vec3 binormal) {
    if (wave.amplitude <= 0.0 || wave.wavelength <= 1.0e-3) {
        return;
    }
    vec2 D = wave.direction;
    float len = length(D);
    if (len > 1.0e-6) {
        D /= len;
    } else {
        D = vec2(1.0, 0.0);
    }
    float k = SPARK_GERSTNER_TWO_PI / wave.wavelength;
    float phase = k * dot(D, worldXZ) - wave.speed * timeSeconds;
    float s = sin(phase);
    float c = cos(phase);
    float Q = wave.steepness;
    float A = wave.amplitude;
    float wA = A * k;

    displacement += vec3(Q * A * D.x * s, A * c, Q * A * D.y * s);

    tangent.x += 1.0 - Q * wA * D.x * D.x * c;
    tangent.y += -wA * D.x * s;
    tangent.z += -Q * wA * D.x * D.y * c;

    binormal.x += -Q * wA * D.x * D.y * c;
    binormal.y += -wA * D.y * s;
    binormal.z += 1.0 - Q * wA * D.y * D.y * c;
}

vec3 sparkGerstnerDisplacement(vec2 worldXZ, float timeSeconds, int waveCount, WaterGerstnerWaveGpu waves[4]) {
    vec3 displacement = vec3(0.0);
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0);
    for (int i = 0; i < waveCount; ++i) {
        sparkGerstnerAccumulate(worldXZ, timeSeconds, waves[i], displacement, tangent, binormal);
    }
    return displacement;
}

vec3 sparkGerstnerNormal(vec2 worldXZ, float timeSeconds, int waveCount, WaterGerstnerWaveGpu waves[4]) {
    vec3 displacement = vec3(0.0);
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0);
    for (int i = 0; i < waveCount; ++i) {
        sparkGerstnerAccumulate(worldXZ, timeSeconds, waves[i], displacement, tangent, binormal);
    }
    return normalize(cross(binormal, tangent));
}

#endif
