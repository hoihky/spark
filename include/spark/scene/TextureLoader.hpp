#pragma once

#include "spark/scene/TextureLoadOptions.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class Texture2D;

/**
 * Facade that dispatches file/memory loads to registered <c>ITextureLoader</c> strategies.
 * <c>Texture2D</c> static load methods delegate here.
 */
class TextureLoader {
public:
    [[nodiscard]] static bool LoadFromFile(const char* path, Texture2D& out, bool flipVerticalOnLoad = true);
    [[nodiscard]] static bool LoadFromKtx2File(const char* path, Texture2D& out);
    [[nodiscard]] static bool LoadFromMemory(
            const std::uint8_t* bytes,
            std::size_t byteCount,
            Texture2D& out,
            const char* debugName = "Memory");
};

}  // namespace Spark
