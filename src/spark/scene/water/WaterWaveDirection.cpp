#include "spark/scene/water/WaterWaveDirection.hpp"

#include <cmath>

namespace Spark {

void WaterWaveDirection::RotateSettingsToPrimarySwell(
        WaterWaveSettings& settings,
        Vector2 travelDirectionWorldXZ) noexcept {
    if (settings.GetActiveWaveCount() == 0) {
        return;
    }

    const float targetLenSq = travelDirectionWorldXZ.x * travelDirectionWorldXZ.x +
                              travelDirectionWorldXZ.y * travelDirectionWorldXZ.y;
    if (targetLenSq < 1.0e-8F) {
        return;
    }
    const float targetInvLen = 1.0F / std::sqrt(targetLenSq);
    const float targetX = travelDirectionWorldXZ.x * targetInvLen;
    const float targetZ = travelDirectionWorldXZ.y * targetInvLen;

    const GerstnerWave& primary = settings.GetWave(0);
    float currentX = primary.direction.x;
    float currentZ = primary.direction.y;
    const float currentLenSq = currentX * currentX + currentZ * currentZ;
    if (currentLenSq < 1.0e-8F) {
        currentX = 1.0F;
        currentZ = 0.0F;
    } else {
        const float currentInvLen = 1.0F / std::sqrt(currentLenSq);
        currentX *= currentInvLen;
        currentZ *= currentInvLen;
    }

    const float currentAngle = std::atan2(currentZ, currentX);
    const float targetAngle = std::atan2(targetZ, targetX);
    const float delta = targetAngle - currentAngle;
    const float cosDelta = std::cos(delta);
    const float sinDelta = std::sin(delta);

    const std::uint32_t count = settings.GetActiveWaveCount();
    for (std::uint32_t wi = 0; wi < count; ++wi) {
        GerstnerWave& wave = settings.GetWave(wi);
        const float dirX = wave.direction.x;
        const float dirZ = wave.direction.y;
        wave.direction.x = dirX * cosDelta - dirZ * sinDelta;
        wave.direction.y = dirX * sinDelta + dirZ * cosDelta;
        const float lenSq = wave.direction.x * wave.direction.x + wave.direction.y * wave.direction.y;
        if (lenSq > 1.0e-8F) {
            const float invLen = 1.0F / std::sqrt(lenSq);
            wave.direction.x *= invLen;
            wave.direction.y *= invLen;
        }
    }
}

}  // namespace Spark
