#include "spark/text/Font.hpp"

#include "spark/core/Utility.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace Spark {

namespace {

constexpr int kFirstChar = 32;
constexpr int kCharCount = 96;

}  // namespace

bool Font::TryLoadTrueTypeFromFile(const char* path, float emPixels) {
    lineAscentPx = 0.0F;
    if (path == nullptr || path[0] == '\0' || emPixels <= 0.0F) {
        return false;
    }
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    const std::streamsize sz = file.tellg();
    if (sz <= 0) {
        return false;
    }
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(static_cast<std::size_t>(sz));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), sz)) {
        return false;
    }

    constexpr int kAtlasW = 512;
    constexpr int kAtlasH = 512;
    Array<std::uint8_t> mono;
    mono.Resize(static_cast<std::size_t>(kAtlasW) * static_cast<std::size_t>(kAtlasH));
    std::memset(mono.GetData(), 0, mono.GetSize());

    packedStorage.Resize(static_cast<std::size_t>(kCharCount) * sizeof(stbtt_packedchar));

    stbtt_pack_context pc{};
    if (stbtt_PackBegin(&pc, mono.GetData(), kAtlasW, kAtlasH, kAtlasW, 1, nullptr) == 0) {
        packedStorage.Clear();
        return false;
    }
    auto* packed = reinterpret_cast<stbtt_packedchar*>(packedStorage.GetData());
    if (stbtt_PackFontRange(
                &pc,
                buffer.data(),
                0,
                STBTT_POINT_SIZE(emPixels),
                kFirstChar,
                kCharCount,
                packed) == 0) {
        stbtt_PackEnd(&pc);
        packedStorage.Clear();
        return false;
    }
    stbtt_PackEnd(&pc);

    lineAscentPx = 0.0F;
    stbtt_fontinfo fi{};
    if (stbtt_InitFont(&fi, buffer.data(), stbtt_GetFontOffsetForIndex(buffer.data(), 0))) {
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
        stbtt_GetFontVMetrics(&fi, &ascent, &descent, &lineGap);
        const float ttScale = stbtt_ScaleForMappingEmToPixels(&fi, STBTT_POINT_SIZE(emPixels));
        lineAscentPx = std::fabs(static_cast<float>(ascent) * ttScale);
    }
    if (lineAscentPx <= 0.0F) {
        lineAscentPx = emPixels * 0.72F;
    }

    Array<std::uint8_t> rgba;
    rgba.Resize(static_cast<std::size_t>(kAtlasW) * static_cast<std::size_t>(kAtlasH) * 4U);
    for (int i = 0; i < kAtlasW * kAtlasH; ++i) {
        const std::uint8_t a = mono[static_cast<std::size_t>(i)];
        const std::size_t o = static_cast<std::size_t>(i) * 4U;
        rgba[o] = 255;
        rgba[o + 1] = 255;
        rgba[o + 2] = 255;
        rgba[o + 3] = a;
    }

    atlas = Texture2D(Utf8String(path));
    atlas.SetPixels(static_cast<std::uint32_t>(kAtlasW), static_cast<std::uint32_t>(kAtlasH), MoveTemp(rgba));
    this->emPixels = emPixels;
    atlasW = kAtlasW;
    atlasH = kAtlasH;
    return true;
}

void Font::AppendTextGeometry(
        const Utf8String& text,
        float x,
        float y,
        float sizePixels,
        const Vector3& color,
        float alpha,
        float atlasLayer,
        Array<float>& interleaved,
        Array<std::uint32_t>& indices) const {
    if (!IsValid() || emPixels <= 0.0F || sizePixels <= 0.0F) {
        return;
    }
    const auto* packed = reinterpret_cast<const stbtt_packedchar*>(packedStorage.GetData());
    const float scale = sizePixels / emPixels;
    float xpos = x / scale;
    // stbtt_GetPackedQuad uses baseline Y; overlays pass top-left (Y down).
    const float ascentAtSize = lineAscentPx * (sizePixels / emPixels);
    float ypos = (y + ascentAtSize) / scale;

    const char* p = text.CStr();
    while (p != nullptr && *p != '\0') {
        unsigned char c = static_cast<unsigned char>(*p++);
        std::uint32_t cp = c;
        if (c >= 0x80U) {
            // Minimal UTF-8: skip continuation / multibyte to next ASCII boundary
            if ((c & 0xE0U) == 0xC0U && *p != '\0') {
                const unsigned char b1 = static_cast<unsigned char>(*p++);
                cp = (static_cast<std::uint32_t>(c & 0x1FU) << 6U) |
                        (static_cast<std::uint32_t>(b1) & 0x3FU);
            } else if ((c & 0xF0U) == 0xE0U && p[0] != '\0' && p[1] != '\0') {
                const unsigned char b1 = static_cast<unsigned char>(*p++);
                const unsigned char b2 = static_cast<unsigned char>(*p++);
                cp = (static_cast<std::uint32_t>(c & 0x0FU) << 12U) |
                        ((static_cast<std::uint32_t>(b1) & 0x3FU) << 6U) |
                        (static_cast<std::uint32_t>(b2) & 0x3FU);
            } else {
                continue;
            }
        }

        if (cp < static_cast<std::uint32_t>(kFirstChar) ||
            cp >= static_cast<std::uint32_t>(kFirstChar + kCharCount)) {
            xpos += emPixels * 0.33F;
            continue;
        }

        stbtt_aligned_quad q{};
        stbtt_GetPackedQuad(
                packed,
                atlasW,
                atlasH,
                static_cast<int>(cp) - kFirstChar,
                &xpos,
                &ypos,
                &q,
                1);

        const float r = color.x;
        const float g = color.y;
        const float b = color.z;
        const std::uint32_t base = static_cast<std::uint32_t>(interleaved.GetSize() / 9U);

        auto pushV = [&](float px, float py, float u, float v) {
            interleaved.PushBack(px * scale);
            interleaved.PushBack(py * scale);
            interleaved.PushBack(u);
            interleaved.PushBack(v);
            interleaved.PushBack(atlasLayer);
            interleaved.PushBack(r);
            interleaved.PushBack(g);
            interleaved.PushBack(b);
            interleaved.PushBack(alpha);
        };

        pushV(q.x0, q.y0, q.s0, q.t0);
        pushV(q.x1, q.y0, q.s1, q.t0);
        pushV(q.x1, q.y1, q.s1, q.t1);
        pushV(q.x0, q.y1, q.s0, q.t1);

        indices.PushBack(base);
        indices.PushBack(base + 1);
        indices.PushBack(base + 2);
        indices.PushBack(base);
        indices.PushBack(base + 2);
        indices.PushBack(base + 3);
    }
}

float Font::MeasureUtf8Width(const Utf8String& text, const float sizePixels) const noexcept {
    if (!IsValid() || emPixels <= 0.0F || sizePixels <= 0.0F) {
        return 0.0F;
    }
    const auto* packed = reinterpret_cast<const stbtt_packedchar*>(packedStorage.GetData());
    const float scale = sizePixels / emPixels;
    float xpos = 0.0F;

    const char* p = text.CStr();
    while (p != nullptr && *p != '\0') {
        unsigned char c = static_cast<unsigned char>(*p++);
        std::uint32_t cp = c;
        if (c >= 0x80U) {
            if ((c & 0xE0U) == 0xC0U && *p != '\0') {
                const unsigned char b1 = static_cast<unsigned char>(*p++);
                cp = (static_cast<std::uint32_t>(c & 0x1FU) << 6U) |
                        (static_cast<std::uint32_t>(b1) & 0x3FU);
            } else if ((c & 0xF0U) == 0xE0U && p[0] != '\0' && p[1] != '\0') {
                const unsigned char b1 = static_cast<unsigned char>(*p++);
                const unsigned char b2 = static_cast<unsigned char>(*p++);
                cp = (static_cast<std::uint32_t>(c & 0x0FU) << 12U) |
                        ((static_cast<std::uint32_t>(b1) & 0x3FU) << 6U) |
                        (static_cast<std::uint32_t>(b2) & 0x3FU);
            } else {
                continue;
            }
        }

        if (cp < static_cast<std::uint32_t>(kFirstChar) ||
            cp >= static_cast<std::uint32_t>(kFirstChar + kCharCount)) {
            xpos += emPixels * 0.33F;
            continue;
        }
        const stbtt_packedchar& ch = packed[static_cast<int>(cp) - kFirstChar];
        xpos += ch.xadvance;
    }
    return xpos * scale;
}

float Font::GetLineSpacingPixels(const float sizePixels) const noexcept {
    if (!IsValid() || emPixels <= 0.0F || sizePixels <= 0.0F) {
        return sizePixels * 1.2F;
    }
    const float scale = sizePixels / emPixels;
    return std::max(lineAscentPx * scale * 1.15F, sizePixels * 1.05F);
}

void Font::BlendUtf8TextOntoRgba(
        std::uint8_t* rgba,
        const std::uint32_t imageWidth,
        const std::uint32_t imageHeight,
        const char* textUtf8,
        float x,
        float y,
        const float sizePixels,
        const std::uint8_t r,
        const std::uint8_t g,
        const std::uint8_t b,
        const std::uint8_t alpha) const {
    if (!IsValid() || rgba == nullptr || imageWidth == 0 || imageHeight == 0 || textUtf8 == nullptr ||
        textUtf8[0] == '\0' || sizePixels <= 0.0F || alpha == 0) {
        return;
    }

    const auto* packed = reinterpret_cast<const stbtt_packedchar*>(packedStorage.GetData());
    const Array<std::uint8_t>& atlasRgba = atlas.GetRgba();
    const float scale = sizePixels / emPixels;
    float xpos = x / scale;
    const float ascentAtSize = lineAscentPx * (sizePixels / emPixels);
    float ypos = (y + ascentAtSize) / scale;
    const float invAlpha = static_cast<float>(alpha) / 255.0F;

    const auto blendPixel = [&](const int px, const int py, const float coverage) {
        if (px < 0 || py < 0 || static_cast<std::uint32_t>(px) >= imageWidth ||
            static_cast<std::uint32_t>(py) >= imageHeight || coverage <= 0.0F) {
            return;
        }
        std::uint8_t* dst = rgba + (static_cast<std::size_t>(py) * static_cast<std::size_t>(imageWidth) +
                                    static_cast<std::size_t>(px)) *
                                           4U;
        const float cov = std::min(1.0F, coverage);
        const float inv = 1.0F - cov;
        dst[0] = static_cast<std::uint8_t>(static_cast<float>(r) * cov + static_cast<float>(dst[0]) * inv);
        dst[1] = static_cast<std::uint8_t>(static_cast<float>(g) * cov + static_cast<float>(dst[1]) * inv);
        dst[2] = static_cast<std::uint8_t>(static_cast<float>(b) * cov + static_cast<float>(dst[2]) * inv);
        dst[3] = 255;
    };

    const auto sampleAtlasAlpha = [&](const float u, const float v) -> float {
        const int ax = std::clamp(static_cast<int>(u * static_cast<float>(atlasW)), 0, atlasW - 1);
        const int ay = std::clamp(static_cast<int>(v * static_cast<float>(atlasH)), 0, atlasH - 1);
        const std::size_t idx = (static_cast<std::size_t>(ay) * static_cast<std::size_t>(atlasW) +
                                 static_cast<std::size_t>(ax)) *
                                4U + 3U;
        return static_cast<float>(atlasRgba[idx]) / 255.0F;
    };

    const char* p = textUtf8;
    while (*p != '\0') {
        unsigned char c = static_cast<unsigned char>(*p++);
        std::uint32_t cp = c;
        if (c >= 0x80U) {
            if ((c & 0xE0U) == 0xC0U && *p != '\0') {
                const unsigned char b1 = static_cast<unsigned char>(*p++);
                cp = (static_cast<std::uint32_t>(c & 0x1FU) << 6U) |
                        (static_cast<std::uint32_t>(b1) & 0x3FU);
            } else if ((c & 0xF0U) == 0xE0U && p[0] != '\0' && p[1] != '\0') {
                const unsigned char b1 = static_cast<unsigned char>(*p++);
                const unsigned char b2 = static_cast<unsigned char>(*p++);
                cp = (static_cast<std::uint32_t>(c & 0x0FU) << 12U) |
                        ((static_cast<std::uint32_t>(b1) & 0x3FU) << 6U) |
                        (static_cast<std::uint32_t>(b2) & 0x3FU);
            } else {
                continue;
            }
        }

        if (cp < static_cast<std::uint32_t>(kFirstChar) ||
            cp >= static_cast<std::uint32_t>(kFirstChar + kCharCount)) {
            xpos += emPixels * 0.33F;
            continue;
        }

        stbtt_aligned_quad q{};
        stbtt_GetPackedQuad(
                packed,
                atlasW,
                atlasH,
                static_cast<int>(cp) - kFirstChar,
                &xpos,
                &ypos,
                &q,
                1);

        const float x0s = q.x0 * scale;
        const float y0s = q.y0 * scale;
        const float x1s = q.x1 * scale;
        const float y1s = q.y1 * scale;
        const int ix0 = static_cast<int>(std::floor(x0s));
        const int iy0 = static_cast<int>(std::floor(y0s));
        const int ix1 = static_cast<int>(std::ceil(x1s));
        const int iy1 = static_cast<int>(std::ceil(y1s));
        const float quadW = x1s - x0s;
        const float quadH = y1s - y0s;
        if (quadW <= 0.0F || quadH <= 0.0F) {
            continue;
        }

        for (int py = iy0; py <= iy1; ++py) {
            for (int px = ix0; px <= ix1; ++px) {
                const float tx = (static_cast<float>(px) + 0.5F - x0s) / quadW;
                const float ty = (static_cast<float>(py) + 0.5F - y0s) / quadH;
                if (tx < 0.0F || ty < 0.0F || tx > 1.0F || ty > 1.0F) {
                    continue;
                }
                const float u = q.s0 + tx * (q.s1 - q.s0);
                const float v = q.t0 + ty * (q.t1 - q.t0);
                const float coverage = sampleAtlasAlpha(u, v) * invAlpha;
                blendPixel(px, py, coverage);
            }
        }
    }
}

}  // namespace Spark
