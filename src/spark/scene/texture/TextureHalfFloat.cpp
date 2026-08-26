#include "spark/scene/texture/TextureHalfFloat.hpp"

#include <cstring>

namespace Spark {

std::uint16_t TextureHalfFloat::FromFloat32(float value) noexcept {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));

    const std::uint32_t sign = (bits >> 16U) & 0x8000U;
    std::int32_t exponent = static_cast<std::int32_t>(((bits >> 23U) & 0xFFU)) - 127 + 15;
    std::uint32_t mantissa = bits & 0x7FFFFFU;

    if (exponent <= 0) {
        if (exponent < -10) {
            return static_cast<std::uint16_t>(sign);
        }
        mantissa |= 0x800000U;
        mantissa >>= static_cast<std::uint32_t>(1 - exponent);
        return static_cast<std::uint16_t>(sign | (mantissa >> 13U));
    }
    if (exponent >= 31) {
        return static_cast<std::uint16_t>(sign | 0x7C00U);
    }
    return static_cast<std::uint16_t>(sign | (static_cast<std::uint32_t>(exponent) << 10U) | (mantissa >> 13U));
}

void TextureHalfFloat::PackRgba16Float(
        const float* const srcRgba,
        std::uint16_t* const dstRgba,
        const std::size_t pixelCount) noexcept {
    if (srcRgba == nullptr || dstRgba == nullptr || pixelCount == 0U) {
        return;
    }
    const std::size_t channelCount = pixelCount * 4U;
    for (std::size_t i = 0; i < channelCount; ++i) {
        dstRgba[i] = FromFloat32(srcRgba[i]);
    }
}

}  // namespace Spark
