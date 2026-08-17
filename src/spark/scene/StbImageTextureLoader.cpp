#include "spark/scene/StbImageTextureLoader.hpp"

#include "spark/core/Utility.hpp"
#include "spark/scene/Texture2D.hpp"

#include "stb_image.h"

#include <cstring>

namespace Spark {

namespace {

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

bool StbImageTextureLoader::CanLoad(const char* path) const {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    return !PathEndsWithInsensitive(path, ".ktx2");
}

bool StbImageTextureLoader::Load(const char* path, Texture2D& out, const TextureLoadOptions& options) const {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    int w = 0;
    int h = 0;
    stbi_set_flip_vertically_on_load(options.GetFlipVerticalOnLoad() ? 1 : 0);
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

bool StbImageTextureLoader::LoadFromMemory(
        const std::uint8_t* bytes,
        const std::size_t byteCount,
        Texture2D& out,
        const char* debugName) {
    if (bytes == nullptr || byteCount == 0) {
        return false;
    }
    int w = 0;
    int h = 0;
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

}  // namespace Spark
