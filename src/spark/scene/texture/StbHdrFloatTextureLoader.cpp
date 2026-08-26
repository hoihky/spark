#include "spark/scene/texture/StbHdrFloatTextureLoader.hpp"

#include "spark/core/Utility.hpp"
#include "spark/scene/texture/Texture2D.hpp"

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

bool StbHdrFloatTextureLoader::CanLoad(const char* path) const {
    return PathEndsWithInsensitive(path, ".hdr");
}

bool StbHdrFloatTextureLoader::Load(const char* path, Texture2D& out, const TextureLoadOptions& options) const {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    int w = 0;
    int h = 0;
    stbi_set_flip_vertically_on_load(options.GetFlipVerticalOnLoad() ? 1 : 0);
    float* data = stbi_loadf(path, &w, &h, nullptr, 4);
    if (data == nullptr || w <= 0 || h <= 0) {
        stbi_image_free(data);
        return false;
    }
    Array<float> pixels;
    const std::size_t n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U;
    pixels.Resize(n);
    std::memcpy(pixels.GetData(), data, n * sizeof(float));
    stbi_image_free(data);
    out = Texture2D(Utf8String(path));
    out.SetSceneUploadNearest(true);
    out.SetFloatPixels(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), MoveTemp(pixels));
    return true;
}

}  // namespace Spark
