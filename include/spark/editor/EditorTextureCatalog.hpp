#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

namespace Spark::Editor {

struct EditorTextureEntry {
    Utf8String displayName{};
    /** Relative path under assets root, e.g. <c>textures/bricks.png</c>. */
    Utf8String relativePath{};
};

/** Scans <c>textures/</c> under build + source asset roots for image files. */
class EditorTextureCatalog final {
public:
    void Refresh();

    [[nodiscard]] const Array<EditorTextureEntry>& GetEntries() const noexcept { return entries; }
    [[nodiscard]] int FindIndexByRelativePath(const char* relativePath) const noexcept;

private:
    void ScanDirectory(const char* assetsRoot, const char* subdir);
    void AddEntry(Utf8String relativePath);

    Array<EditorTextureEntry> entries{};
};

}  // namespace Spark::Editor
