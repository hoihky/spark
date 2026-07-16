#include "spark/scene/Texture2D.hpp"

#include "spark/core/ContentFingerprint.hpp"
#include "spark/core/Utility.hpp"

#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Spark {

Texture2D::Texture2D(Utf8String textureName) : name(MoveTemp(textureName)) {}

void Texture2D::RefreshContentFingerprint() noexcept {
    std::uint64_t h = Fnv64Begin();
    Fnv64Mix(h, static_cast<std::uint64_t>(width));
    Fnv64Mix(h, static_cast<std::uint64_t>(height));
    Fnv64Mix(h, static_cast<std::uint64_t>(rgba.GetSize()));
    if (!rgba.IsEmpty()) {
        h = Fnv64HashBytes(h, rgba.GetData(), rgba.GetSize());
    }
    contentFingerprint = h;
}

void Texture2D::SetPixels(std::uint32_t w, std::uint32_t h, Array<std::uint8_t> bytes) {
    width = w;
    height = h;
    rgba = MoveTemp(bytes);
    RefreshContentFingerprint();
}

void Texture2D::ResampleNearest(std::uint32_t targetW, std::uint32_t targetH, Array<std::uint8_t>& outRgba) const {
    if (width == 0 || height == 0 || targetW == 0 || targetH == 0) {
        outRgba.Clear();
        return;
    }
    outRgba.Clear();
    outRgba.Resize(static_cast<std::size_t>(targetW) * static_cast<std::size_t>(targetH) * 4U);
    for (std::uint32_t y = 0; y < targetH; ++y) {
        for (std::uint32_t x = 0; x < targetW; ++x) {
            const std::uint32_t sx = std::min((x * width) / std::max(targetW, 1U), width - 1U);
            const std::uint32_t sy = std::min((y * height) / std::max(targetH, 1U), height - 1U);
            const std::size_t si =
                    (static_cast<std::size_t>(sy) * static_cast<std::size_t>(width) + static_cast<std::size_t>(sx)) *
                    4U;
            const std::size_t di =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(targetW) + static_cast<std::size_t>(x)) *
                    4U;
            outRgba[di] = rgba[si];
            outRgba[di + 1] = rgba[si + 1];
            outRgba[di + 2] = rgba[si + 2];
            outRgba[di + 3] = rgba[si + 3];
        }
    }
}

void Texture2D::ResampleBilinear(std::uint32_t targetW, std::uint32_t targetH, Array<std::uint8_t>& outRgba) const {
    if (width == 0 || height == 0 || targetW == 0 || targetH == 0) {
        outRgba.Clear();
        return;
    }
    if (width == targetW && height == targetH) {
        outRgba = rgba;
        return;
    }
    outRgba.Clear();
    outRgba.Resize(static_cast<std::size_t>(targetW) * static_cast<std::size_t>(targetH) * 4U);
    for (std::uint32_t y = 0; y < targetH; ++y) {
        const float v =
                (static_cast<float>(y) + 0.5F) * static_cast<float>(height) / static_cast<float>(targetH) - 0.5F;
        const float sy = std::clamp(v, 0.0F, static_cast<float>(height - 1U));
        const std::uint32_t y0 = static_cast<std::uint32_t>(std::floor(sy));
        const std::uint32_t y1 = std::min(y0 + 1U, height - 1U);
        const float fy = sy - static_cast<float>(y0);
        for (std::uint32_t x = 0; x < targetW; ++x) {
            const float u =
                    (static_cast<float>(x) + 0.5F) * static_cast<float>(width) / static_cast<float>(targetW) - 0.5F;
            const float sx = std::clamp(u, 0.0F, static_cast<float>(width - 1U));
            const std::uint32_t x0 = static_cast<std::uint32_t>(std::floor(sx));
            const std::uint32_t x1 = std::min(x0 + 1U, width - 1U);
            const float fx = sx - static_cast<float>(x0);
            auto sample = [&](const std::uint32_t sampleX, const std::uint32_t sampleY) -> const std::uint8_t* {
                const std::size_t si =
                        (static_cast<std::size_t>(sampleY) * static_cast<std::size_t>(width) +
                         static_cast<std::size_t>(sampleX)) *
                        4U;
                return &rgba[si];
            };
            const std::uint8_t* c00 = sample(x0, y0);
            const std::uint8_t* c10 = sample(x1, y0);
            const std::uint8_t* c01 = sample(x0, y1);
            const std::uint8_t* c11 = sample(x1, y1);
            const std::size_t di =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(targetW) + static_cast<std::size_t>(x)) *
                    4U;
            for (int channel = 0; channel < 4; ++channel) {
                const float top = static_cast<float>(c00[channel]) + fx * static_cast<float>(c10[channel] - c00[channel]);
                const float bot = static_cast<float>(c01[channel]) + fx * static_cast<float>(c11[channel] - c01[channel]);
                outRgba[di + static_cast<std::size_t>(channel)] =
                        static_cast<std::uint8_t>(std::clamp(top + fy * (bot - top), 0.0F, 255.0F));
            }
        }
    }
}

Texture2D Texture2D::CreateCheckerboard(
        std::uint32_t size,
        std::uint32_t tilePixels,
        Vector3 colorA,
        Vector3 colorB) {
    Texture2D t(Utf8String("Checkerboard"));
    const std::uint32_t tp = std::max(tilePixels, 1U);
    Array<std::uint8_t> bytes;
    bytes.Resize(static_cast<std::size_t>(size) * static_cast<std::size_t>(size) * 4U);
    for (std::uint32_t y = 0; y < size; ++y) {
        for (std::uint32_t x = 0; x < size; ++x) {
            const bool dark = (((x / tp) + (y / tp)) % 2U) == 0U;
            const Vector3 c = dark ? colorA : colorB;
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(size) + x) * 4U;
            bytes[i] = static_cast<std::uint8_t>(std::clamp(c.x * 255.0F, 0.0F, 255.0F));
            bytes[i + 1] = static_cast<std::uint8_t>(std::clamp(c.y * 255.0F, 0.0F, 255.0F));
            bytes[i + 2] = static_cast<std::uint8_t>(std::clamp(c.z * 255.0F, 0.0F, 255.0F));
            bytes[i + 3] = 255;
        }
    }
    t.SetPixels(size, size, MoveTemp(bytes));
    return t;
}

Texture2D Texture2D::CreateSolid(std::uint32_t w, std::uint32_t h, Vector3 rgb, float alpha) {
    Texture2D t(Utf8String("Solid"));
    Array<std::uint8_t> bytes;
    bytes.Resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    const auto r = static_cast<std::uint8_t>(std::clamp(rgb.x * 255.0F, 0.0F, 255.0F));
    const auto g = static_cast<std::uint8_t>(std::clamp(rgb.y * 255.0F, 0.0F, 255.0F));
    const auto b = static_cast<std::uint8_t>(std::clamp(rgb.z * 255.0F, 0.0F, 255.0F));
    const auto a = static_cast<std::uint8_t>(std::clamp(alpha * 255.0F, 0.0F, 255.0F));
    for (std::size_t i = 0; i < bytes.GetSize(); i += 4) {
        bytes[i] = r;
        bytes[i + 1] = g;
        bytes[i + 2] = b;
        bytes[i + 3] = a;
    }
    t.SetPixels(w, h, MoveTemp(bytes));
    return t;
}

bool Texture2D::TryLoadFromMemory(
        const std::uint8_t* bytes, std::size_t byteCount, Texture2D& out, const char* debugName) {
    if (bytes == nullptr || byteCount == 0) {
        return false;
    }
    int w = 0;
    int h = 0;
    // glTF (and Vulkan image uploads) expect row 0 = top of the texture; stbi flip=1 makes row 0 = bottom-left.
    stbi_set_flip_vertically_on_load(0);
    unsigned char* data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(bytes), static_cast<int>(byteCount), &w, &h, nullptr, 4);
    if (data == nullptr || w <= 0 || h <= 0) {
        stbi_image_free(data);
        return false;
    }
    Array<std::uint8_t> rgbaBytes;
    const std::size_t n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U;
    rgbaBytes.Resize(n);
    std::memcpy(rgbaBytes.GetData(), data, n);
    stbi_image_free(data);
    const char* nm = (debugName != nullptr && debugName[0] != '\0') ? debugName : "Memory";
    out = Texture2D(Utf8String(nm));
    out.SetPixels(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), MoveTemp(rgbaBytes));
    return true;
}

bool Texture2D::TryLoadFromFile(const char* path, Texture2D& out, const bool flipVerticalOnLoad) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    int w = 0;
    int h = 0;
    stbi_set_flip_vertically_on_load(flipVerticalOnLoad ? 1 : 0);
    unsigned char* data = stbi_load(path, &w, &h, nullptr, 4);
    if (data == nullptr || w <= 0 || h <= 0) {
        stbi_image_free(data);
        return false;
    }
    Array<std::uint8_t> bytes;
    const std::size_t n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U;
    bytes.Resize(n);
    std::memcpy(bytes.GetData(), data, n);
    stbi_image_free(data);
    out = Texture2D(Utf8String(path));
    out.SetPixels(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), MoveTemp(bytes));
    return true;
}

Texture2D Texture2D::CreateBrickPattern(std::uint32_t width, std::uint32_t height) {
    Texture2D t(Utf8String("BrickPattern"));
    const std::uint32_t bw = std::max(width / 4U, 8U);
    const std::uint32_t bh = std::max(height / 8U, 6U);
    const std::uint32_t mw = std::max(bw / 16U, 1U);
    const std::uint32_t mh = std::max(bh / 12U, 1U);
    Array<std::uint8_t> bytes;
    bytes.Resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint32_t row = y / (bh + mh);
            const std::uint32_t ox = ((row % 2U) * (bw / 2U)) % (bw + mw);
            const std::uint32_t bx = (x + ox) % (bw + mw);
            const std::uint32_t by = y % (bh + mh);
            const bool mortarH = by >= bh;
            const bool mortarV = bx >= bw;
            std::uint8_t r = 0;
            std::uint8_t g = 0;
            std::uint8_t b = 0;
            if (mortarH || mortarV) {
                r = 140;
                g = 132;
                b = 128;
            } else {
                const std::uint32_t brickId = (x + ox) / (bw + mw) + row;
                const bool alt = (brickId % 2U) == 0U;
                if (alt) {
                    r = 165;
                    g = 72;
                    b = 52;
                } else {
                    r = 148;
                    g = 58;
                    b = 40;
                }
            }
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + x) * 4U;
            bytes[i] = r;
            bytes[i + 1] = g;
            bytes[i + 2] = b;
            bytes[i + 3] = 255;
        }
    }
    t.SetPixels(width, height, MoveTemp(bytes));
    return t;
}

Texture2D Texture2D::CreateSoilPattern(std::uint32_t width, std::uint32_t height) {
    Texture2D t(Utf8String("SoilPattern"));
    const auto soilHash = [](std::uint32_t x, std::uint32_t y) -> std::uint32_t {
        std::uint32_t h = x * 374761393U + y * 668265263U;
        h = (h ^ (h >> 13U)) * 1274126177U;
        return h ^ (h >> 16U);
    };
    Array<std::uint8_t> bytes;
    bytes.Resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint32_t cx = x / 7U;
            const std::uint32_t cy = y / 7U;
            const std::uint32_t n1 = soilHash(cx, cy);
            const std::uint32_t n2 = soilHash(cx + 19U, cy + 31U);
            const std::uint32_t n3 = soilHash(x, y);
            const float blend =
                    (static_cast<float>(n1 & 255U) * 0.45F + static_cast<float>(n2 & 255U) * 0.35F +
                            static_cast<float>(n3 & 63U) * 0.31F) /
                    255.0F;
            const Vector3 dark{0.26F, 0.18F, 0.12F};
            const Vector3 mid{0.42F, 0.30F, 0.19F};
            const Vector3 light{0.55F, 0.44F, 0.28F};
            Vector3 c{};
            if (blend < 0.38F) {
                const float u = blend / 0.38F;
                c.x = dark.x + (mid.x - dark.x) * u;
                c.y = dark.y + (mid.y - dark.y) * u;
                c.z = dark.z + (mid.z - dark.z) * u;
            } else {
                const float u = (blend - 0.38F) / 0.62F;
                c.x = mid.x + (light.x - mid.x) * u;
                c.y = mid.y + (light.y - mid.y) * u;
                c.z = mid.z + (light.z - mid.z) * u;
            }
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + x) * 4U;
            bytes[i] = static_cast<std::uint8_t>(std::clamp(c.x * 255.0F, 0.0F, 255.0F));
            bytes[i + 1] = static_cast<std::uint8_t>(std::clamp(c.y * 255.0F, 0.0F, 255.0F));
            bytes[i + 2] = static_cast<std::uint8_t>(std::clamp(c.z * 255.0F, 0.0F, 255.0F));
            bytes[i + 3] = 255;
        }
    }
    t.SetPixels(width, height, MoveTemp(bytes));
    return t;
}

Texture2D Texture2D::CreateVariedSoilGroundPattern(const std::uint32_t width, const std::uint32_t height) {
    Texture2D t(Utf8String("VariedSoilGround"));
    const auto h32 = [](std::uint32_t x, std::uint32_t y) -> std::uint32_t {
        std::uint32_t v = x * 0x9E3779B1u ^ y * 0x85EBCA6Bu;
        v ^= v << 13U;
        v ^= v >> 17U;
        v ^= v << 5U;
        return v;
    };
    const auto h01 = [&h32](std::uint32_t x, std::uint32_t y) -> float {
        return static_cast<float>(h32(x, y) & 0xFFFFFFu) / static_cast<float>(0x1000000u);
    };
    const auto smoothCell = [&h01](std::uint32_t x, std::uint32_t y, std::uint32_t cell) -> float {
        const std::uint32_t cx = x / cell;
        const std::uint32_t cy = y / cell;
        const float fx = static_cast<float>(x % cell) / static_cast<float>(cell);
        const float fy = static_cast<float>(y % cell) / static_cast<float>(cell);
        const float u = fx * fx * (3.0F - 2.0F * fx);
        const float v = fy * fy * (3.0F - 2.0F * fy);
        const float a00 = h01(cx, cy);
        const float a10 = h01(cx + 1U, cy);
        const float a01 = h01(cx, cy + 1U);
        const float a11 = h01(cx + 1U, cy + 1U);
        const float ab = a00 + (a10 - a00) * u;
        const float cd = a01 + (a11 - a01) * u;
        return ab + (cd - ab) * v;
    };
    const auto ramp = [](const Vector3& dark, const Vector3& mid, const Vector3& light, float s) -> Vector3 {
        Vector3 o{};
        if (s < 0.4F) {
            const float k = s / 0.4F;
            o.x = dark.x + (mid.x - dark.x) * k;
            o.y = dark.y + (mid.y - dark.y) * k;
            o.z = dark.z + (mid.z - dark.z) * k;
        } else {
            const float k = (s - 0.4F) / 0.6F;
            o.x = mid.x + (light.x - mid.x) * k;
            o.y = mid.y + (light.y - mid.y) * k;
            o.z = mid.z + (light.z - mid.z) * k;
        }
        return o;
    };

    Array<std::uint8_t> bytes;
    bytes.Resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const float patchA = smoothCell(x, y, 86U);
            const float patchB = smoothCell(x + 31U, y + 17U, 53U);
            const float patchC = smoothCell(x + 7U, y + 59U, 29U);
            float w0 = 0.28F + patchA * 0.72F;
            float w1 = 0.28F + patchB * 0.72F;
            float w2 = 0.28F + patchC * 0.72F;
            const float w3 = 0.15F + smoothCell(x + 101U, y, 71U) * 0.85F;
            const float inv = 1.0F / (w0 + w1 + w2 + w3);

            const std::uint32_t n1 = h32(x / 2U, y / 2U);
            const std::uint32_t n2 = h32(x + 13U, y + 29U);
            const std::uint32_t n3 = h32(x * 11U + y, y * 5U + x);
            const float fine =
                    (static_cast<float>(n1 & 255U) * 0.36F + static_cast<float>(n2 & 255U) * 0.36F +
                            static_cast<float>(n3 & 127U) * 0.28F) /
                    255.0F;

            const Vector3 loamD{0.22F, 0.15F, 0.10F};
            const Vector3 loamM{0.40F, 0.28F, 0.18F};
            const Vector3 loamL{0.54F, 0.42F, 0.28F};
            const Vector3 clayD{0.38F, 0.18F, 0.12F};
            const Vector3 clayM{0.52F, 0.26F, 0.16F};
            const Vector3 clayL{0.62F, 0.34F, 0.22F};
            const Vector3 sandD{0.38F, 0.32F, 0.22F};
            const Vector3 sandM{0.52F, 0.44F, 0.30F};
            const Vector3 sandL{0.64F, 0.56F, 0.38F};
            const Vector3 siltD{0.24F, 0.22F, 0.20F};
            const Vector3 siltM{0.36F, 0.34F, 0.31F};
            const Vector3 siltL{0.46F, 0.44F, 0.40F};

            Vector3 c0 = ramp(loamD, loamM, loamL, fine);
            Vector3 c1 = ramp(clayD, clayM, clayL, fine * 0.92F + patchB * 0.08F);
            Vector3 c2 = ramp(sandD, sandM, sandL, fine * 0.88F + patchA * 0.12F);
            Vector3 c3 = ramp(siltD, siltM, siltL, fine);

            Vector3 c{};
            c.x = (c0.x * w0 + c1.x * w1 + c2.x * w2 + c3.x * w3) * inv;
            c.y = (c0.y * w0 + c1.y * w1 + c2.y * w2 + c3.y * w3) * inv;
            c.z = (c0.z * w0 + c1.z * w1 + c2.z * w2 + c3.z * w3) * inv;

            const float streak = 0.5F
                    + 0.5F * std::sin(static_cast<float>(x) * 0.09F + static_cast<float>(y) * 0.11F + patchC * 5.5F);
            const float shade = 0.88F + 0.12F * streak;
            c.x *= shade;
            c.y *= shade;
            c.z *= shade;

            if ((h32(x, y) & 0x1FFu) > 498U) {
                c.x *= 0.82F;
                c.y *= 0.82F;
                c.z *= 0.82F;
            }

            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + x) * 4U;
            bytes[i] = static_cast<std::uint8_t>(std::clamp(c.x * 255.0F, 0.0F, 255.0F));
            bytes[i + 1U] = static_cast<std::uint8_t>(std::clamp(c.y * 255.0F, 0.0F, 255.0F));
            bytes[i + 2U] = static_cast<std::uint8_t>(std::clamp(c.z * 255.0F, 0.0F, 255.0F));
            bytes[i + 3U] = 255;
        }
    }
    t.SetPixels(width, height, MoveTemp(bytes));
    return t;
}

}  // namespace Spark
