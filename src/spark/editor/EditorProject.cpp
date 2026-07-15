#include "spark/editor/EditorProject.hpp"

#include <cstdio>

namespace Spark::Editor {

bool EditorProject::CreateNewAt(const char* const projectRootUtf8, const WorkspaceDimension dimension) noexcept {
    if (projectRootUtf8 == nullptr || projectRootUtf8[0] == '\0') {
        return false;
    }
    settings_ = {};
    settings_.rootDirectory = Utf8String(projectRootUtf8);
    settings_.workspace = dimension;
    settings_.projectName = Utf8String("New Project");
    isOpen_ = true;
    dirty_ = true;
    return TrySaveProjectFile();
}

bool EditorProject::OpenExisting(const char* const projectRootUtf8) noexcept {
    if (projectRootUtf8 == nullptr || projectRootUtf8[0] == '\0') {
        return false;
    }
    settings_ = {};
    settings_.rootDirectory = Utf8String(projectRootUtf8);
    settings_.workspace = WorkspaceDimension::ThreeD;
    settings_.projectName = Utf8String("Opened Project");
    isOpen_ = true;
    dirty_ = false;
    return isOpen_;
}

bool EditorProject::TrySaveProjectFile() noexcept {
    if (!isOpen_ || settings_.rootDirectory.IsEmpty()) {
        return false;
    }
    Utf8String path = settings_.rootDirectory;
    if (!path.IsEmpty() && path.CStr()[path.ByteLength() - 1] != '/') {
        path.AppendUtf8("/");
    }
    path.AppendUtf8("project.spark");

    FILE* f = std::fopen(path.CStr(), "wb");
    if (f == nullptr) {
        return false;
    }
    const char* dim = settings_.workspace == WorkspaceDimension::TwoD ? "2d" : "3d";
    std::fprintf(
            f,
            "spark_project_v1\n"
            "name=%s\n"
            "workspace=%s\n"
            "assets=%s\n"
            "scenes=%s\n"
            "main_scene=%s\n",
            settings_.projectName.CStr(),
            dim,
            settings_.assetsDirectory.CStr(),
            settings_.scenesDirectory.CStr(),
            settings_.mainScenePath.CStr());
    std::fclose(f);
    dirty_ = false;
    return true;
}

}  // namespace Spark::Editor
