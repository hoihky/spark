#pragma once

#include "spark/core/Utf8String.hpp"

namespace Spark {

/** Cross-platform native file picker (Strategy); macOS uses NSOpenPanel. */
class NativeFilePicker final {
public:
    /**
     * Opens a modal glTF/glB file picker.
     * @return false when cancelled or unsupported on the current platform.
     */
    [[nodiscard]] static bool TryPickGltfFile(Utf8String& outAbsolutePath) noexcept;

private:
    NativeFilePicker() = delete;
};

}  // namespace Spark
