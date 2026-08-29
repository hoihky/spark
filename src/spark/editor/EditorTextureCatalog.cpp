#include "spark/editor/EditorTextureCatalog.hpp"

#include "spark/config.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

namespace Spark::Editor {

namespace {

bool IsTextureExtension(const char* name) noexcept {
    if (name == nullptr) {
        return false;
    }
    const char* dot = std::strrchr(name, '.');
    if (dot == nullptr) {
        return false;
    }
    const char* ext = dot + 1;
    auto ieq = [](const char* a, const char* b) {
        while (*a != '\0' && *b != '\0') {
            if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
                return false;
            }
            ++a;
            ++b;
        }
        return *a == '\0' && *b == '\0';
    };
    return ieq(ext, "png") || ieq(ext, "jpg") || ieq(ext, "jpeg") || ieq(ext, "hdr") || ieq(ext, "tga");
}

Utf8String HumanizeFileName(const std::string& fileName) {
    const std::size_t dot = fileName.find_last_of('.');
    const std::string stem = dot == std::string::npos ? fileName : fileName.substr(0, dot);
    Utf8String out{};
    bool capitalizeNext = true;
    for (char ch : stem) {
        if (ch == '_' || ch == '-') {
            out.AppendUtf8(" ");
            capitalizeNext = true;
            continue;
        }
        if (capitalizeNext && std::isalpha(static_cast<unsigned char>(ch)) != 0) {
            const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            const char buf[2]{upper, '\0'};
            out.AppendUtf8(buf);
            capitalizeNext = false;
        } else {
            const char buf[2]{ch, '\0'};
            out.AppendUtf8(buf);
            capitalizeNext = false;
        }
    }
    return out;
}

}  // namespace

void EditorTextureCatalog::Refresh() {
    entries.Clear();
    ScanDirectory(SPARK_BUILD_ASSETS_DIR, "textures");
    ScanDirectory(SPARK_ASSETS_DIR, "textures");

    if (entries.GetSize() > 1) {
        std::sort(
                entries.GetData(),
                entries.GetData() + entries.GetSize(),
                [](const EditorTextureEntry& a, const EditorTextureEntry& b) {
                    return std::strcmp(a.displayName.CStr(), b.displayName.CStr()) < 0;
                });
    }
}

void EditorTextureCatalog::ScanDirectory(const char* const assetsRoot, const char* const subdir) {
    if (assetsRoot == nullptr || subdir == nullptr) {
        return;
    }
    std::filesystem::path root(assetsRoot);
    root /= subdir;
    std::error_code ec{};
    if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string fileName = entry.path().filename().string();
        if (!IsTextureExtension(fileName.c_str())) {
            continue;
        }
        const std::filesystem::path rel = std::filesystem::relative(entry.path(), std::filesystem::path(assetsRoot), ec);
        if (ec) {
            continue;
        }
        Utf8String relativePath(rel.generic_string().c_str());
        AddEntry(MoveTemp(relativePath));
    }
}

void EditorTextureCatalog::AddEntry(Utf8String relativePath) {
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (entries[i].relativePath == relativePath) {
            return;
        }
    }
    EditorTextureEntry entry{};
    entry.relativePath = MoveTemp(relativePath);
    const char* slash = std::strrchr(entry.relativePath.CStr(), '/');
    const char* fileName = slash != nullptr ? slash + 1 : entry.relativePath.CStr();
    entry.displayName = HumanizeFileName(fileName);
    entries.PushBack(MoveTemp(entry));
}

int EditorTextureCatalog::FindIndexByRelativePath(const char* const relativePath) const noexcept {
    if (relativePath == nullptr || relativePath[0] == '\0') {
        return 0;
    }
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        if (entries[i].relativePath == Utf8String(relativePath)) {
            return static_cast<int>(i) + 1;
        }
    }
    return 0;
}

}  // namespace Spark::Editor
