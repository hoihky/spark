#include "spark/scene/Ktx2TextureLoader.hpp"

#include "spark/core/Utility.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/scene/TextureFormat.hpp"
#include "spark/scene/Texture2D.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace Spark {

namespace {

constexpr std::uint8_t kKtx2Identifier[12] = {
        0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};

[[nodiscard]] bool ReadFileBytes(const char* path, std::vector<std::uint8_t>& out) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    std::FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return false;
    }
    const long fileSize = std::ftell(file);
    if (fileSize <= 0) {
        std::fclose(file);
        return false;
    }
    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }
    out.resize(static_cast<std::size_t>(fileSize));
    const std::size_t readCount = std::fread(out.data(), 1, out.size(), file);
    std::fclose(file);
    return readCount == out.size();
}

[[nodiscard]] std::uint32_t ReadU32Le(const std::uint8_t* bytes) noexcept {
    return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[2]) << 16U) | (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

[[nodiscard]] std::uint64_t ReadU64Le(const std::uint8_t* bytes) noexcept {
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<std::uint64_t>(bytes[i]) << (static_cast<std::uint64_t>(i) * 8U);
    }
    return value;
}

[[nodiscard]] TexturePixelFormat VkFormatToPixelFormat(const std::uint32_t vkFormat) noexcept {
    // VkFormat values for the block formats Spark accepts in KTX2.
    constexpr std::uint32_t kVkBc7Unorm = 145;
    constexpr std::uint32_t kVkAstc4x4Unorm = 158;
    if (vkFormat == kVkBc7Unorm) {
        return TexturePixelFormat::Bc7Unorm;
    }
    if (vkFormat == kVkAstc4x4Unorm) {
        return TexturePixelFormat::Astc4x4Unorm;
    }
    return TexturePixelFormat::Rgba8Unorm;
}

[[nodiscard]] bool PathEndsWithInsensitive(const char* path, const char* suffix) {
    if (path == nullptr || suffix == nullptr) {
        return false;
    }
    const std::size_t pathLen = std::strlen(path);
    const std::size_t suffixLen = std::strlen(suffix);
    if (pathLen < suffixLen) {
        return false;
    }
    const char* tail = path + pathLen - suffixLen;
    for (std::size_t i = 0; i < suffixLen; ++i) {
        char a = tail[i];
        char b = suffix[i];
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b - 'A' + 'a');
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool Ktx2TextureLoader::CanLoad(const char* path) const {
    return PathEndsWithInsensitive(path, ".ktx2");
}

bool Ktx2TextureLoader::Load(const char* path, Texture2D& out, const TextureLoadOptions&) const {
    return LoadFromFile(path, out);
}

bool Ktx2TextureLoader::LoadFromFile(const char* path, Texture2D& out) {
    std::vector<std::uint8_t> fileBytes;
    if (!ReadFileBytes(path, fileBytes) || fileBytes.size() < 80) {
        return false;
    }
    if (std::memcmp(fileBytes.data(), kKtx2Identifier, sizeof(kKtx2Identifier)) != 0) {
        return false;
    }

    const std::uint32_t vkFormat = ReadU32Le(fileBytes.data() + 12);
    const std::uint32_t pixelWidth = ReadU32Le(fileBytes.data() + 20);
    const std::uint32_t pixelHeight = ReadU32Le(fileBytes.data() + 24);
    const std::uint32_t levelCount = ReadU32Le(fileBytes.data() + 40);
    const std::uint32_t supercompression = ReadU32Le(fileBytes.data() + 44);
    if (supercompression != 0 || levelCount == 0 || pixelWidth == 0 || pixelHeight == 0) {
        return false;
    }

    const TexturePixelFormat format = VkFormatToPixelFormat(vkFormat);
    if (format == TexturePixelFormat::Rgba8Unorm) {
        return false;
    }

    constexpr std::size_t kHeaderBytes = 80;
    constexpr std::size_t kIndexStride = 24;
    if (fileBytes.size() < kHeaderBytes + levelCount * kIndexStride) {
        return false;
    }

    Array<TextureMipLevel> mips;
    mips.Reserve(levelCount);
    for (std::uint32_t level = 0; level < levelCount; ++level) {
        const std::size_t indexOffset = kHeaderBytes + static_cast<std::size_t>(level) * kIndexStride;
        const std::uint64_t byteOffset = ReadU64Le(fileBytes.data() + indexOffset);
        const std::uint64_t byteLength = ReadU64Le(fileBytes.data() + indexOffset + 8);
        if (byteLength == 0 || byteOffset + byteLength > fileBytes.size()) {
            return false;
        }
        Array<std::uint8_t> levelBytes;
        levelBytes.Resize(static_cast<std::size_t>(byteLength));
        std::memcpy(levelBytes.GetData(), fileBytes.data() + byteOffset, static_cast<std::size_t>(byteLength));
        mips.PushBack(TextureMipLevel(
                TextureFormat::MipDimension(pixelWidth, level),
                TextureFormat::MipDimension(pixelHeight, level),
                MoveTemp(levelBytes)));
    }

    out = Texture2D(Utf8String(path));
    out.SetCompressedMipChain(format, pixelWidth, pixelHeight, MoveTemp(mips));
    return true;
}

}  // namespace Spark
