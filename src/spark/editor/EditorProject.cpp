#include "spark/editor/EditorProject.hpp"

#include <cstdio>

namespace Spark::Editor {

bool EditorProject::CreateNewAt(const char* const projectRootUtf8, const WorkspaceDimension dimension) noexcept {
    if (projectRootUtf8 == nullptr || projectRootUtf8[0] == '\0') {
        return false;
    }
    settings = {};
    settings.rootDirectory = Utf8String(projectRootUtf8);
    settings.workspace = dimension;
    settings.projectName = Utf8String("New Project");
    isOpen = true;
    dirty = true;
    return TrySaveProjectFile();
}

bool EditorProject::OpenExisting(const char* const projectRootUtf8) noexcept {
    if (projectRootUtf8 == nullptr || projectRootUtf8[0] == '\0') {
        return false;
    }
    settings = {};
    settings.rootDirectory = Utf8String(projectRootUtf8);
    settings.workspace = WorkspaceDimension::ThreeD;
    settings.projectName = Utf8String("Opened Project");
    isOpen = true;
    dirty = false;
    return isOpen;
}

bool EditorProject::TrySaveProjectFile() noexcept {
    if (!isOpen || settings.rootDirectory.IsEmpty()) {
        return false;
    }
    Utf8String path = settings.rootDirectory;
    if (!path.IsEmpty() && path.CStr()[path.ByteLength() - 1] != '/') {
        path.AppendUtf8("/");
    }
    path.AppendUtf8("project.spark");

    FILE* f = std::fopen(path.CStr(), "wb");
    if (f == nullptr) {
        return false;
    }
    const char* dim = settings.workspace == WorkspaceDimension::TwoD ? "2d" : "3d";
    std::fprintf(
            f,
            "spark_project_v1\n"
            "name=%s\n"
            "workspace=%s\n"
            "assets=%s\n"
            "scenes=%s\n"
            "main_scene=%s\n",
            settings.projectName.CStr(),
            dim,
            settings.assetsDirectory.CStr(),
            settings.scenesDirectory.CStr(),
            settings.mainScenePath.CStr());
    std::fclose(f);
    dirty = false;
    return true;
}

}  // namespace Spark::Editor
