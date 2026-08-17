#pragma once

namespace Spark {

class Texture2D;
class TextureLoadOptions;

/** Strategy interface for decoding texture files into <c>Texture2D</c>. */
class ITextureLoader {
public:
    virtual ~ITextureLoader() = default;

    [[nodiscard]] virtual bool CanLoad(const char* path) const = 0;
    [[nodiscard]] virtual bool Load(const char* path, Texture2D& out, const TextureLoadOptions& options) const = 0;
};

}  // namespace Spark
