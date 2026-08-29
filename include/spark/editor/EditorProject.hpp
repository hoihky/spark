#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorTypes.hpp"

namespace Spark::Editor {

/** On-disk <c>project.spark</c> descriptor (JSON v1). */
struct EditorProjectSettings {
    Utf8String projectName{"Untitled"};
    Utf8String rootDirectory;
    /** Relative to project root; use <c>.</c> when the project root is the assets folder. */
    Utf8String assetsDirectory{"."};
    Utf8String scenesDirectory{"scenes"};
    Utf8String mainScenePath{"scenes/main.sparkscene"};
    WorkspaceDimension workspace = WorkspaceDimension::ThreeD;
};

class EditorProject {
public:
    [[nodiscard]] const EditorProjectSettings& GetSettings() const noexcept { return settings; }
    [[nodiscard]] EditorProjectSettings& GetSettings() noexcept { return settings; }

    /** Creates default folder layout under @p projectRoot. */
    [[nodiscard]] bool CreateNewAt(const char* projectRootUtf8, WorkspaceDimension dimension) noexcept;

    /** Opens an existing folder; loads <c>project.spark</c> when present. */
    [[nodiscard]] bool OpenExisting(const char* projectRootUtf8) noexcept;

    [[nodiscard]] bool IsOpen() const noexcept { return isOpen; }
    [[nodiscard]] bool IsDirty() const noexcept { return dirty; }
    void MarkDirty() noexcept { dirty = true; }
    void ClearDirty() noexcept { dirty = false; }

    [[nodiscard]] bool TrySaveProjectFile() noexcept;

    /** Updates the on-disk project folder without reloading <c>project.spark</c>. */
    [[nodiscard]] bool RelocateRoot(const char* projectRootUtf8) noexcept;

    /** Absolute path to the content root (project root + assets directory). */
    [[nodiscard]] Utf8String GetAssetsRootAbsolute() const noexcept;

    /** Absolute path to <c>project.spark</c> in the open project. */
    [[nodiscard]] Utf8String GetProjectFilePath() const noexcept;

private:
    [[nodiscard]] bool TryLoadProjectFile() noexcept;
    [[nodiscard]] bool TryLoadLegacyProjectFile(const char* path) noexcept;
    [[nodiscard]] bool TryLoadJsonProjectFile(const char* path) noexcept;

    EditorProjectSettings settings{};
    bool isOpen = false;
    bool dirty = false;
};

}  // namespace Spark::Editor
