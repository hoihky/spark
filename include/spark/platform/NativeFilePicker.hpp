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

    /**
     * Opens a modal folder picker for project roots.
     * @return false when cancelled or unsupported on the current platform.
     */
    [[nodiscard]] static bool TryPickProjectFolder(Utf8String& outAbsolutePath) noexcept;

    /**
     * Folder picker for saving a project to a chosen location.
     * @return false when cancelled or unsupported on the current platform.
     */
    [[nodiscard]] static bool TryPickSaveProjectFolder(Utf8String& outAbsolutePath) noexcept;

    /**
     * Open-panel for <c>.sparkscene</c> files.
     * @param defaultDirectory optional folder to open the panel in
     */
    [[nodiscard]] static bool TryPickOpenSparkSceneFile(
            Utf8String& outAbsolutePath,
            const char* defaultDirectory = nullptr) noexcept;

    /**
     * Save-panel for <c>.sparkscene</c> files.
     * @param suggestedFileName e.g. <c>editor_session.sparkscene</c>
     * @param defaultDirectory optional folder to open the panel in
     */
    [[nodiscard]] static bool TryPickSaveSparkSceneFile(
            Utf8String& outAbsolutePath,
            const char* suggestedFileName,
            const char* defaultDirectory = nullptr) noexcept;

private:
    NativeFilePicker() = delete;
};

}  // namespace Spark
