#pragma once

namespace Spark {

constexpr float Pi = 3.14159265358979323846F;
constexpr float TwoPi = Pi * 2.0F;
constexpr float HalfPi = Pi * 0.5F;
constexpr float InvPi = 1.0F / Pi;
constexpr float Epsilon = 1.0e-6F;

constexpr float DegreesToRadians(float degrees) noexcept { return degrees * (Pi / 180.0F); }
constexpr float RadiansToDegrees(float radians) noexcept { return radians * (180.0F / Pi); }

}  // namespace Spark
