#include "spark/scene/prefab/PrefabCatalog.hpp"

#include <cstring>

namespace Spark {

void PrefabCatalog::Register(PrefabCatalogEntry entry) {
    if (entry.fileName.IsEmpty()) {
        return;
    }
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (std::strcmp(entries[i].fileName.CStr(), entry.fileName.CStr()) == 0) {
            entries[i] = MoveTemp(entry);
            return;
        }
    }
    entries.PushBack(MoveTemp(entry));
}

void PrefabCatalog::Clear() noexcept {
    entries.Clear();
}

const PrefabCatalogEntry* PrefabCatalog::FindByFileName(const char* fileName) const noexcept {
    if (fileName == nullptr || fileName[0] == '\0') {
        return nullptr;
    }
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (std::strcmp(entries[i].fileName.CStr(), fileName) == 0) {
            return &entries[i];
        }
    }
    return nullptr;
}

void PrefabCatalog::RegisterDemoDefaults(PrefabCatalog& catalog) {
    catalog.Register(PrefabCatalogEntry{
            .menuLabel = Utf8String("Prefab — Crate"),
            .fileName = Utf8String("crate.sparkscene"),
            .captureMeshHint = Utf8String("builtin:unit_cube"),
    });
    catalog.Register(PrefabCatalogEntry{
            .menuLabel = Utf8String("Prefab — Barrel"),
            .fileName = Utf8String("barrel.sparkscene"),
            .captureMeshHint = Utf8String("builtin:unit_cube"),
    });
}

}  // namespace Spark
