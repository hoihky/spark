#include "spark/scene/editor/SceneEditorAssetCatalog.hpp"

#include "spark/config.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cstring>

namespace Spark {

namespace {

bool EndsWithSparkScene(const char* name) noexcept {
    if (name == nullptr) {
        return false;
    }
    const std::size_t len = std::strlen(name);
    return len >= 11 && std::strcmp(name + len - 11, ".sparkscene") == 0;
}

Utf8String HumanizeStem(const std::string& stem) {
    if (stem.empty()) {
        return {};
    }
    Utf8String out;
    bool capitalizeNext = true;
    for (char ch : stem) {
        if (ch == '_' || ch == '-') {
            out.AppendUtf8(" ");
            capitalizeNext = true;
            continue;
        }
        if (capitalizeNext && std::isalpha(static_cast<unsigned char>(ch)) != 0) {
            const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            char buf[2]{upper, '\0'};
            out.AppendUtf8(buf);
            capitalizeNext = false;
        } else {
            char buf[2]{ch, '\0'};
            out.AppendUtf8(buf);
            capitalizeNext = false;
        }
    }
    return out;
}

}  // namespace

void SceneEditorAssetCatalog::Refresh() {
    entries.Clear();
    listIndexToEntry.Clear();
    ScanDirectory(SPARK_BUILD_ASSETS_DIR, "prefabs", SceneEditorAssetKind::Prefab);
    ScanDirectory(SPARK_ASSETS_DIR, "prefabs", SceneEditorAssetKind::Prefab);
    ScanDirectory(SPARK_BUILD_ASSETS_DIR, "scenes", SceneEditorAssetKind::Scene);
    ScanDirectory(SPARK_ASSETS_DIR, "scenes", SceneEditorAssetKind::Scene);

    if (entries.GetSize() > 1) {
        std::sort(
                entries.GetData(),
                entries.GetData() + entries.GetSize(),
                [](const SceneEditorAssetEntry& a, const SceneEditorAssetEntry& b) {
        if (a.kind != b.kind) {
            return static_cast<int>(a.kind) < static_cast<int>(b.kind);
        }
        return std::strcmp(a.displayName.CStr(), b.displayName.CStr()) < 0;
                });
    }

    listIndexToEntry.Resize(entries.GetSize());
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        listIndexToEntry[i] = static_cast<int>(i);
    }
}

Array<SceneEditorAssetEntry> SceneEditorAssetCatalog::GetEntriesByKind(const SceneEditorAssetKind kind) const {
    Array<SceneEditorAssetEntry> filtered;
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (entries[i].kind == kind) {
            filtered.PushBack(entries[i]);
        }
    }
    return filtered;
}

const SceneEditorAssetEntry* SceneEditorAssetCatalog::FindByListIndex(const int listIndex) const noexcept {
    if (listIndex < 0 || static_cast<std::size_t>(listIndex) >= listIndexToEntry.GetSize()) {
        return nullptr;
    }
    const int entryIndex = listIndexToEntry[static_cast<std::size_t>(listIndex)];
    if (entryIndex < 0 || static_cast<std::size_t>(entryIndex) >= entries.GetSize()) {
        return nullptr;
    }
    return &entries[static_cast<std::size_t>(entryIndex)];
}

void SceneEditorAssetCatalog::ScanDirectory(
        const char* assetsRoot,
        const char* subdir,
        const SceneEditorAssetKind kind) {
    if (assetsRoot == nullptr || subdir == nullptr) {
        return;
    }
    std::error_code ec{};
    const std::filesystem::path dir = std::filesystem::path(assetsRoot) / subdir;
    if (!std::filesystem::is_directory(dir, ec)) {
        return;
    }
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec || !entry.is_regular_file()) {
            continue;
        }
        const std::string fileName = entry.path().filename().string();
        if (!EndsWithSparkScene(fileName.c_str())) {
            continue;
        }
        Utf8String rel(subdir);
        rel.AppendUtf8("/");
        rel.AppendUtf8(fileName.c_str());
        bool duplicate = false;
        for (std::size_t i = 0; i < entries.GetSize(); ++i) {
            if (entries[i].relativePath == rel) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }
        AddEntry(kind, Utf8String(fileName.c_str()), MoveTemp(rel));
    }
}

void SceneEditorAssetCatalog::AddEntry(
        const SceneEditorAssetKind kind,
        Utf8String fileName,
        Utf8String relativePath) {
    SceneEditorAssetEntry entry{};
    entry.kind = kind;
    entry.fileName = MoveTemp(fileName);
    entry.relativePath = MoveTemp(relativePath);
    std::string stem = std::filesystem::path(entry.fileName.CStr()).stem().string();
    entry.displayName = HumanizeStem(stem);
    if (entry.displayName.IsEmpty()) {
        entry.displayName = entry.fileName;
    }
    Utf8String prefix = kind == SceneEditorAssetKind::Prefab ? Utf8String("[Prefab] ") : Utf8String("[Scene] ");
    prefix.AppendUtf8(entry.displayName.CStr());
    entry.displayName = MoveTemp(prefix);
    entries.PushBack(MoveTemp(entry));
}

}  // namespace Spark
