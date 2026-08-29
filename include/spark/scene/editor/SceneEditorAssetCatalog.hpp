#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

namespace Spark {

/** Kind of asset exposed in the scene-editor browser. */
enum class SceneEditorAssetKind {
    Prefab,
    Scene,
};

/** One row in the asset browser (prefab or scene file). */
struct SceneEditorAssetEntry {
    SceneEditorAssetKind kind = SceneEditorAssetKind::Prefab;
    Utf8String displayName{};
    /** Relative path under assets root, e.g. <c>prefabs/crate.sparkscene</c>. */
    Utf8String relativePath{};
    Utf8String fileName{};
};

/**
 * Scans <c>prefabs/</c> and <c>scenes/</c> under build + source asset roots.
 * Registry pattern: refresh repopulates the in-memory catalog.
 */
class SceneEditorAssetCatalog final {
public:
    void Refresh();

    [[nodiscard]] const Array<SceneEditorAssetEntry>& GetEntries() const noexcept { return entries; }
    [[nodiscard]] Array<SceneEditorAssetEntry> GetEntriesByKind(const SceneEditorAssetKind kind) const;

    [[nodiscard]] const SceneEditorAssetEntry* FindByListIndex(int listIndex) const noexcept;

private:
    void ScanDirectory(const char* assetsRoot, const char* subdir, SceneEditorAssetKind kind);
    void AddEntry(SceneEditorAssetKind kind, Utf8String fileName, Utf8String relativePath);

    Array<SceneEditorAssetEntry> entries{};
    Array<int> listIndexToEntry{};
};

}  // namespace Spark
