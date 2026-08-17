#pragma once

#include "spark/scene/ITextureLoader.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/** Loads PNG/JPEG and other formats supported by stb_image into RGBA8. */
class StbImageTextureLoader final : public ITextureLoader {
public:
    [[nodiscard]] bool CanLoad(const char* path) const override;
    [[nodiscard]] bool Load(const char* path, Texture2D& out, const TextureLoadOptions& options) const override;

    [[nodiscard]] static bool LoadFromMemory(
            const std::uint8_t* bytes,
            std::size_t byteCount,
            Texture2D& out,
            const char* debugName);
};

}  // namespace Spark
