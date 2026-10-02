#include "spark/scene/foliage/GrassInstanceTintGenerator.hpp"

namespace Spark {

namespace {

Vector3 LerpRgb(const Vector3& a, const Vector3& b, const float t) noexcept {
    return a + (b - a) * t;
}

}  // namespace

Vector4 GrassInstanceTintGenerator::BuildTint(
        const Vector3& baseTint,
        const float hashA,
        const float hashB,
        const float hashC) noexcept {
    const Vector3 darkOlive{0.28F, 0.62F, 0.18F};
    const Vector3 meadowGreen{0.42F, 0.92F, 0.34F};
    const Vector3 yellowGreen{0.58F, 0.9F, 0.26F};
    const Vector3 sunBleached{0.62F, 0.82F, 0.38F};

    Vector3 palette = LerpRgb(darkOlive, meadowGreen, hashA);
    palette = LerpRgb(palette, yellowGreen, hashB * 0.55F);
    palette = LerpRgb(palette, sunBleached, hashC * 0.35F);

    const float brightness = 0.78F + hashA * 0.38F;
    const Vector3 rgb{
            palette.x * baseTint.x * brightness,
            palette.y * baseTint.y * brightness,
            palette.z * baseTint.z * brightness};
    return Vector4{rgb.x, rgb.y, rgb.z, 1.0F};
}

}  // namespace Spark
