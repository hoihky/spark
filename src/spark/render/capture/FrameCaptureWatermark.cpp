#include "spark/render/capture/FrameCaptureWatermark.hpp"

#include "spark/config.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/text/Font.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

constexpr const char* kFrameCaptureWatermarkText = "@ 2026 Spark Engine All rights reserved";

const Font& WatermarkFont() {
    static Font font;
    static bool triedLoad = false;
    if (!triedLoad) {
        triedLoad = true;
        (void)font.TryLoadTrueTypeFromFile(SPARK_UI_FONT_PATH, 36.0F);
    }
    return font;
}

}  // namespace

void ApplyFrameCaptureWatermark(std::uint8_t* rgba, const std::uint32_t width, const std::uint32_t height) noexcept {
    const Font& font = WatermarkFont();
    if (!font.IsValid() || rgba == nullptr || width == 0 || height == 0) {
        return;
    }

    const float margin = std::max(12.0F, static_cast<float>(height) * 0.018F);
    const float sizePx = std::clamp(static_cast<float>(height) * 0.022F, 14.0F, 28.0F);
    const float textW = font.MeasureUtf8Width(Utf8String(kFrameCaptureWatermarkText), sizePx);
    const float x = static_cast<float>(width) - margin - textW;
    const float y = static_cast<float>(height) - margin - sizePx;

    font.BlendUtf8TextOntoRgba(rgba, width, height, kFrameCaptureWatermarkText, x + 1.0F, y + 1.0F, sizePx, 0, 0, 0, 140);
    font.BlendUtf8TextOntoRgba(rgba, width, height, kFrameCaptureWatermarkText, x, y, sizePx, 255, 255, 255, 210);
}

void ConvertBgraRowsToRgba(
        const std::uint8_t* bgraRows,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::size_t bgraRowPitchBytes,
        std::uint8_t* rgbaOut) noexcept {
    if (bgraRows == nullptr || rgbaOut == nullptr || width == 0 || height == 0) {
        return;
    }
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* row = bgraRows + y * bgraRowPitchBytes;
        std::uint8_t* dstRow = rgbaOut + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4U;
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint8_t b = row[x * 4 + 0];
            const std::uint8_t g = row[x * 4 + 1];
            const std::uint8_t r = row[x * 4 + 2];
            const std::uint8_t a = row[x * 4 + 3];
            dstRow[x * 4 + 0] = r;
            dstRow[x * 4 + 1] = g;
            dstRow[x * 4 + 2] = b;
            dstRow[x * 4 + 3] = a;
        }
    }
}

void ConvertRgbaToBgraRows(
        const std::uint8_t* rgba,
        const std::uint32_t width,
        const std::uint32_t height,
        std::uint8_t* bgraOut,
        const std::size_t bgraRowPitchBytes) noexcept {
    if (rgba == nullptr || bgraOut == nullptr || width == 0 || height == 0) {
        return;
    }
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* srcRow = rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4U;
        std::uint8_t* dstRow = bgraOut + y * bgraRowPitchBytes;
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint8_t r = srcRow[x * 4 + 0];
            const std::uint8_t g = srcRow[x * 4 + 1];
            const std::uint8_t b = srcRow[x * 4 + 2];
            const std::uint8_t a = srcRow[x * 4 + 3];
            dstRow[x * 4 + 0] = b;
            dstRow[x * 4 + 1] = g;
            dstRow[x * 4 + 2] = r;
            dstRow[x * 4 + 3] = a;
        }
    }
}

void DownscaleBgraBox(
        const std::uint8_t* srcBgra,
        const std::uint32_t srcWidth,
        const std::uint32_t srcHeight,
        std::uint8_t* dstBgra,
        const std::uint32_t dstWidth,
        const std::uint32_t dstHeight) noexcept {
    if (srcBgra == nullptr || dstBgra == nullptr || srcWidth == 0 || srcHeight == 0 || dstWidth == 0 || dstHeight == 0) {
        return;
    }
    if (srcWidth == dstWidth && srcHeight == dstHeight) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(srcWidth) * static_cast<std::size_t>(srcHeight) * 4U; ++i) {
            dstBgra[i] = srcBgra[i];
        }
        return;
    }
    for (std::uint32_t dy = 0; dy < dstHeight; ++dy) {
        const std::uint32_t sy0 = (dy * srcHeight) / dstHeight;
        const std::uint32_t sy1 = ((dy + 1U) * srcHeight) / dstHeight;
        for (std::uint32_t dx = 0; dx < dstWidth; ++dx) {
            const std::uint32_t sx0 = (dx * srcWidth) / dstWidth;
            const std::uint32_t sx1 = ((dx + 1U) * srcWidth) / dstWidth;
            std::uint32_t r = 0;
            std::uint32_t g = 0;
            std::uint32_t b = 0;
            std::uint32_t a = 0;
            std::uint32_t count = 0;
            for (std::uint32_t sy = sy0; sy < sy1; ++sy) {
                for (std::uint32_t sx = sx0; sx < sx1; ++sx) {
                    const std::size_t idx = (static_cast<std::size_t>(sy) * static_cast<std::size_t>(srcWidth) + sx) * 4U;
                    b += srcBgra[idx + 0];
                    g += srcBgra[idx + 1];
                    r += srcBgra[idx + 2];
                    a += srcBgra[idx + 3];
                    ++count;
                }
            }
            if (count == 0) {
                count = 1;
            }
            const std::size_t didx = (static_cast<std::size_t>(dy) * static_cast<std::size_t>(dstWidth) + dx) * 4U;
            dstBgra[didx + 0] = static_cast<std::uint8_t>(b / count);
            dstBgra[didx + 1] = static_cast<std::uint8_t>(g / count);
            dstBgra[didx + 2] = static_cast<std::uint8_t>(r / count);
            dstBgra[didx + 3] = static_cast<std::uint8_t>(a / count);
        }
    }
}

}  // namespace Spark
