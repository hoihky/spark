#pragma once

#include <cstddef>
#include <cstdint>

namespace Spark {

/** Bottom-right watermark shared by PNG screenshots and MP4 recordings. */
void ApplyFrameCaptureWatermark(std::uint8_t* rgba, std::uint32_t width, std::uint32_t height) noexcept;

/** Copy BGRA rows (possibly padded) into tightly packed RGBA. */
void ConvertBgraRowsToRgba(
        const std::uint8_t* bgraRows,
        std::uint32_t width,
        std::uint32_t height,
        std::size_t bgraRowPitchBytes,
        std::uint8_t* rgbaOut) noexcept;

/** RGBA → BGRA tightly packed rows. */
void ConvertRgbaToBgraRows(
        const std::uint8_t* rgba,
        std::uint32_t width,
        std::uint32_t height,
        std::uint8_t* bgraOut,
        std::size_t bgraRowPitchBytes) noexcept;

/** Box-filter downscale BGRA (source tightly packed). */
void DownscaleBgraBox(
        const std::uint8_t* srcBgra,
        std::uint32_t srcWidth,
        std::uint32_t srcHeight,
        std::uint8_t* dstBgra,
        std::uint32_t dstWidth,
        std::uint32_t dstHeight) noexcept;

}  // namespace Spark
