#pragma once

#include "spark/scene/ITextureLoader.hpp"

namespace Spark {

/** Loads raw KTX2 files with BC7 or ASTC 4x4 mip payloads (supercompression scheme 0). */
class Ktx2TextureLoader final : public ITextureLoader {
public:
    [[nodiscard]] bool CanLoad(const char* path) const override;
    [[nodiscard]] bool Load(const char* path, Texture2D& out, const TextureLoadOptions& options) const override;

    [[nodiscard]] static bool LoadFromFile(const char* path, Texture2D& out);
};

}  // namespace Spark
