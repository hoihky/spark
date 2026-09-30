#include "spark/scene/foliage/GrassFoliageAlbedoTexture.hpp"

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

SharedPtr<Texture2D> GrassFoliageAlbedoTexture::CreateSharedBladeAlbedo() {
    constexpr std::uint32_t kWidth = 32U;
    constexpr std::uint32_t kHeight = 128U;
    Array<std::uint8_t> pixels;
    pixels.Resize(static_cast<std::size_t>(kWidth) * static_cast<std::size_t>(kHeight) * 4U);

    for (std::uint32_t y = 0; y < kHeight; ++y) {
        const float v = static_cast<float>(y) / static_cast<float>(kHeight - 1U);
        const float heightFromBase = 1.0F - v;
        for (std::uint32_t x = 0; x < kWidth; ++x) {
            const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(kWidth);
            const float center = std::abs(u - 0.5F) * 2.0F;
            const float widthMask = 1.0F - std::pow(center, 1.65F);

            const float baseToTip = std::pow(heightFromBase, 0.55F);
            float alpha = widthMask * baseToTip;
            alpha *= std::clamp((heightFromBase - 0.04F) / 0.25F, 0.0F, 1.0F);
            const float tipSoft = 1.0F - std::pow(v, 2.2F);
            alpha *= tipSoft;

            const float r = (0.12F + 0.18F * heightFromBase) * widthMask;
            const float g = (0.42F + 0.48F * heightFromBase) * widthMask;
            const float b = (0.10F + 0.14F * (1.0F - heightFromBase)) * widthMask;

            const std::size_t i =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(kWidth) + static_cast<std::size_t>(x)) * 4U;
            pixels[i + 0U] = static_cast<std::uint8_t>(std::clamp(r * 255.0F, 0.0F, 255.0F));
            pixels[i + 1U] = static_cast<std::uint8_t>(std::clamp(g * 255.0F, 0.0F, 255.0F));
            pixels[i + 2U] = static_cast<std::uint8_t>(std::clamp(b * 255.0F, 0.0F, 255.0F));
            pixels[i + 3U] = static_cast<std::uint8_t>(std::clamp(alpha * 255.0F, 0.0F, 255.0F));
        }
    }

    SharedPtr<Texture2D> texture = MakeShared<Texture2D>(Utf8String("GrassBladeAlbedo"));
    texture->SetPixels(kWidth, kHeight, MoveTemp(pixels));
    return texture;
}

}  // namespace Spark
