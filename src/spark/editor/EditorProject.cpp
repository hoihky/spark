#include "spark/editor/EditorProject.hpp"

#include "spark/core/Array.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace Spark::Editor {

namespace {

Utf8String JoinPath(const Utf8String& root, const char* relative) {
    if (root.IsEmpty() || relative == nullptr || relative[0] == '\0') {
        return root;
    }
    Utf8String out = root;
    if (!out.IsEmpty() && out.CStr()[out.ByteLength() - 1] != '/') {
        out.AppendUtf8("/");
    }
    while (relative[0] == '/') {
        ++relative;
    }
    out.AppendUtf8(relative);
    return out;
}

void JsonEscape(const char* in, char* out, std::size_t outSize) {
    if (outSize == 0) {
        return;
    }
    std::size_t w = 0;
    if (in == nullptr) {
        out[0] = '\0';
        return;
    }
    for (; *in != '\0' && w + 2 < outSize; ++in) {
        const char ch = *in;
        if (ch == '"' || ch == '\\') {
            if (w + 3 >= outSize) {
                break;
            }
            out[w++] = '\\';
        }
        out[w++] = ch;
    }
    out[w] = '\0';
}

const char* SkipWs(const char* p) {
    while (p != nullptr && *p != '\0' && std::isspace(static_cast<unsigned char>(*p)) != 0) {
        ++p;
    }
    return p;
}

bool ReadJsonStringValue(const char* json, const char* key, char* out, std::size_t outSize) {
    if (json == nullptr || key == nullptr || out == nullptr || outSize == 0) {
        return false;
    }
    char pattern[128]{};
    std::snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* keyPos = std::strstr(json, pattern);
    if (keyPos == nullptr) {
        return false;
    }
    const char* colon = std::strchr(keyPos + std::strlen(pattern), ':');
    if (colon == nullptr) {
        return false;
    }
    const char* p = SkipWs(colon + 1);
    if (p == nullptr || *p != '"') {
        return false;
    }
    ++p;
    std::size_t w = 0;
    while (*p != '\0' && *p != '"') {
        if (*p == '\\' && p[1] != '\0') {
            ++p;
        }
        if (w + 1 < outSize) {
            out[w++] = *p;
        }
        ++p;
    }
    out[w] = '\0';
    return w > 0;
}

bool ReadJsonIntValue(const char* json, const char* key, int& out) {
    char pattern[128]{};
    std::snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* keyPos = std::strstr(json, pattern);
    if (keyPos == nullptr) {
        return false;
    }
    const char* colon = std::strchr(keyPos + std::strlen(pattern), ':');
    if (colon == nullptr) {
        return false;
    }
    const char* p = SkipWs(colon + 1);
    return p != nullptr && std::sscanf(p, "%d", &out) == 1;
}

bool EnsureDirectory(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        return std::filesystem::is_directory(path, ec);
    }
    return std::filesystem::create_directories(path, ec);
}

}  // namespace

Utf8String EditorProject::GetProjectFilePath() const noexcept {
    return JoinPath(settings.rootDirectory, "project.spark");
}

Utf8String EditorProject::GetAssetsRootAbsolute() const noexcept {
    if (std::strcmp(settings.assetsDirectory.CStr(), ".") == 0
            || std::strcmp(settings.assetsDirectory.CStr(), "./") == 0) {
        return settings.rootDirectory;
    }
    return JoinPath(settings.rootDirectory, settings.assetsDirectory.CStr());
}

bool EditorProject::CreateNewAt(const char* const projectRootUtf8, const WorkspaceDimension dimension) noexcept {
    if (projectRootUtf8 == nullptr || projectRootUtf8[0] == '\0') {
        return false;
    }
    settings = {};
    settings.rootDirectory = Utf8String(projectRootUtf8);
    settings.workspace = dimension;
    settings.projectName = Utf8String("New Project");
    settings.assetsDirectory = Utf8String("assets");
    settings.scenesDirectory = Utf8String("scenes");
    settings.mainScenePath = Utf8String("scenes/main.sparkscene");

    const std::filesystem::path root(projectRootUtf8);
    if (!EnsureDirectory(root)) {
        return false;
    }
    if (!EnsureDirectory(root / "assets")) {
        return false;
    }
    if (!EnsureDirectory(root / "assets" / "prefabs")) {
        return false;
    }
    if (!EnsureDirectory(root / "assets" / "scenes")) {
        return false;
    }
    if (!EnsureDirectory(root / "scenes")) {
        return false;
    }

    isOpen = true;
    dirty = true;
    return TrySaveProjectFile();
}

bool EditorProject::TryLoadLegacyProjectFile(const char* const path) noexcept {
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return false;
    }
    char line[512]{};
    if (std::fgets(line, sizeof(line), f) == nullptr || std::strncmp(line, "spark_project_v1", 16) != 0) {
        std::fclose(f);
        return false;
    }
    while (std::fgets(line, sizeof(line), f) != nullptr) {
        char value[384]{};
        if (std::sscanf(line, "name=%383[^\n]", value) == 1) {
            settings.projectName = Utf8String(value);
        } else if (std::sscanf(line, "workspace=%383[^\n]", value) == 1) {
            settings.workspace = (std::strcmp(value, "2d") == 0) ? WorkspaceDimension::TwoD : WorkspaceDimension::ThreeD;
        } else if (std::sscanf(line, "assets=%383[^\n]", value) == 1) {
            settings.assetsDirectory = Utf8String(value);
        } else if (std::sscanf(line, "scenes=%383[^\n]", value) == 1) {
            settings.scenesDirectory = Utf8String(value);
        } else if (std::sscanf(line, "main_scene=%383[^\n]", value) == 1) {
            settings.mainScenePath = Utf8String(value);
        }
    }
    std::fclose(f);
    return true;
}

bool EditorProject::TryLoadJsonProjectFile(const char* const path) noexcept {
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    const long fileSize = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (fileSize <= 0 || fileSize > 65536) {
        std::fclose(f);
        return false;
    }
    Array<char> buffer;
    buffer.Resize(static_cast<std::size_t>(fileSize) + 1U);
    const std::size_t read = std::fread(buffer.GetData(), 1U, static_cast<std::size_t>(fileSize), f);
    std::fclose(f);
    buffer[read] = '\0';
    const char* json = buffer.GetData();
    if (json[0] != '{') {
        return false;
    }

    char value[512]{};
    if (ReadJsonStringValue(json, "name", value, sizeof(value))) {
        settings.projectName = Utf8String(value);
    }
    if (ReadJsonStringValue(json, "workspace", value, sizeof(value))) {
        settings.workspace = (std::strcmp(value, "2d") == 0) ? WorkspaceDimension::TwoD : WorkspaceDimension::ThreeD;
    }
    if (ReadJsonStringValue(json, "assets", value, sizeof(value))) {
        settings.assetsDirectory = Utf8String(value);
    }
    if (ReadJsonStringValue(json, "scenes", value, sizeof(value))) {
        settings.scenesDirectory = Utf8String(value);
    }
    if (ReadJsonStringValue(json, "main_scene", value, sizeof(value))) {
        settings.mainScenePath = Utf8String(value);
    }
    int version = 0;
    (void)ReadJsonIntValue(json, "version", version);
    return true;
}

bool EditorProject::TryLoadProjectFile() noexcept {
    const Utf8String path = GetProjectFilePath();
    if (path.IsEmpty()) {
        return false;
    }
    if (TryLoadJsonProjectFile(path.CStr())) {
        return true;
    }
    return TryLoadLegacyProjectFile(path.CStr());
}

bool EditorProject::OpenExisting(const char* const projectRootUtf8) noexcept {
    if (projectRootUtf8 == nullptr || projectRootUtf8[0] == '\0') {
        return false;
    }
    settings = {};
    settings.rootDirectory = Utf8String(projectRootUtf8);
    settings.workspace = WorkspaceDimension::ThreeD;
    settings.projectName = Utf8String("Opened Project");
    settings.assetsDirectory = Utf8String(".");
    settings.scenesDirectory = Utf8String("scenes");
    settings.mainScenePath = Utf8String("scenes/editor_session.sparkscene");
    isOpen = true;
    dirty = false;

    if (TryLoadProjectFile()) {
        return true;
    }

    const std::filesystem::path root(projectRootUtf8);
    std::error_code ec;
    if (std::filesystem::is_directory(root / "assets", ec)) {
        settings.assetsDirectory = Utf8String("assets");
        settings.mainScenePath = Utf8String("assets/scenes/main.sparkscene");
    }
    return isOpen;
}

bool EditorProject::RelocateRoot(const char* const projectRootUtf8) noexcept {
    if (projectRootUtf8 == nullptr || projectRootUtf8[0] == '\0') {
        return false;
    }
    settings.rootDirectory = Utf8String(projectRootUtf8);
    isOpen = true;
    dirty = true;
    return true;
}

bool EditorProject::TrySaveProjectFile() noexcept {
    if (!isOpen || settings.rootDirectory.IsEmpty()) {
        return false;
    }
    const Utf8String path = GetProjectFilePath();
    if (path.IsEmpty()) {
        return false;
    }

    std::FILE* f = std::fopen(path.CStr(), "wb");
    if (f == nullptr) {
        return false;
    }

    char escapedName[512]{};
    char escapedAssets[256]{};
    char escapedScenes[256]{};
    char escapedMain[512]{};
    JsonEscape(settings.projectName.CStr(), escapedName, sizeof(escapedName));
    JsonEscape(settings.assetsDirectory.CStr(), escapedAssets, sizeof(escapedAssets));
    JsonEscape(settings.scenesDirectory.CStr(), escapedScenes, sizeof(escapedScenes));
    JsonEscape(settings.mainScenePath.CStr(), escapedMain, sizeof(escapedMain));
    const char* dim = settings.workspace == WorkspaceDimension::TwoD ? "2d" : "3d";

    std::fprintf(
            f,
            "{\n"
            "  \"version\": 1,\n"
            "  \"name\": \"%s\",\n"
            "  \"workspace\": \"%s\",\n"
            "  \"assets\": \"%s\",\n"
            "  \"scenes\": \"%s\",\n"
            "  \"main_scene\": \"%s\"\n"
            "}\n",
            escapedName,
            dim,
            escapedAssets,
            escapedScenes,
            escapedMain);
    std::fclose(f);
    dirty = false;
    return true;
}

}  // namespace Spark::Editor
