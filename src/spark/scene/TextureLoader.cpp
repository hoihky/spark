#include "spark/scene/TextureLoader.hpp"

#include "spark/scene/Ktx2TextureLoader.hpp"
#include "spark/scene/StbImageTextureLoader.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/scene/TextureLoadOptions.hpp"

namespace Spark {

namespace {

const Ktx2TextureLoader& Ktx2Loader() {
    static const Ktx2TextureLoader loader;
    return loader;
}

const StbImageTextureLoader& StbLoader() {
    static const StbImageTextureLoader loader;
    return loader;
}

}  // namespace

bool TextureLoader::LoadFromFile(const char* path, Texture2D& out, const bool flipVerticalOnLoad) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (Ktx2Loader().CanLoad(path)) {
        return Ktx2Loader().Load(path, out, TextureLoadOptions{}.SetFlipVerticalOnLoad(flipVerticalOnLoad));
    }
    return StbLoader().Load(path, out, TextureLoadOptions{}.SetFlipVerticalOnLoad(flipVerticalOnLoad));
}

bool TextureLoader::LoadFromKtx2File(const char* path, Texture2D& out) {
    return Ktx2TextureLoader::LoadFromFile(path, out);
}

bool TextureLoader::LoadFromMemory(
        const std::uint8_t* bytes,
        const std::size_t byteCount,
        Texture2D& out,
        const char* debugName) {
    return StbImageTextureLoader::LoadFromMemory(bytes, byteCount, out, debugName);
}

}  // namespace Spark
