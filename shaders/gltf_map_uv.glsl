vec2 sparkGltfMapUv(vec2 uv0, vec2 uv1, int setIndex, vec2 scale, vec2 offset, float rotation) {
    vec2 uv = (setIndex == 1) ? uv1 : uv0;
    float c = cos(rotation);
    float s = sin(rotation);
    vec2 scaled = uv * scale;
    mat2 rot = mat2(c, -s, s, c);
    return rot * scaled + offset;
}
