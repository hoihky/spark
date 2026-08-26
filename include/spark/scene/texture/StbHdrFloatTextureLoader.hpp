#pragma once

#include "spark/scene/texture/ITextureLoader.hpp"

namespace Spark {

/** Loads Radiance .hdr files via stbi_loadf into linear RGBA32F <c>Texture2D</c> pixels. */
class StbHdrFloatTextureLoader final : public ITextureLoader {
public:
    [[nodiscard]] bool CanLoad(const char* path) const override;
    [[nodiscard]] bool Load(const char* path, Texture2D& out, const TextureLoadOptions& options) const override;
};

}  // namespace Spark
