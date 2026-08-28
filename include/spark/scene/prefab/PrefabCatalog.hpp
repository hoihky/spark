#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"

namespace Spark {

/** Metadata for a registered <c>.sparkscene</c> prefab (Registry entry). */
struct PrefabCatalogEntry {
    Utf8String menuLabel{};
    Utf8String fileName{};
    /** Hint stored on placed entities for save round-trip (e.g. prefabs/crate.sparkscene). */
    Utf8String captureMeshHint{};
};

/**
 * Extensible catalog of designer-facing prefabs.
 * Games and demos register entries at startup; placement actions query by file name.
 */
class PrefabCatalog final {
public:
    void Register(PrefabCatalogEntry entry);
    void Clear() noexcept;

    [[nodiscard]] const Array<PrefabCatalogEntry>& GetEntries() const noexcept { return entries; }
    [[nodiscard]] const PrefabCatalogEntry* FindByFileName(const char* fileName) const noexcept;

    /** Demo defaults: crate + barrel under <c>prefabs/</c>. */
    static void RegisterDemoDefaults(PrefabCatalog& catalog);

private:
    Array<PrefabCatalogEntry> entries{};
};

}  // namespace Spark
