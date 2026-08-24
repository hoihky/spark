#pragma once

namespace Spark {

/** Options passed to texture file/memory loaders. */
class TextureLoadOptions {
public:
    TextureLoadOptions() = default;

    [[nodiscard]] bool GetFlipVerticalOnLoad() const noexcept { return flipVerticalOnLoad; }
    TextureLoadOptions& SetFlipVerticalOnLoad(const bool flip) noexcept {
        flipVerticalOnLoad = flip;
        return *this;
    }

    [[nodiscard]] const char* GetDebugName() const noexcept { return debugName; }
    TextureLoadOptions& SetDebugName(const char* name) noexcept {
        debugName = name;
        return *this;
    }

private:
    bool flipVerticalOnLoad = true;
    const char* debugName = "Memory";
};

}  // namespace Spark
