#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorTypes.hpp"

namespace Spark::Editor {

/** Minimal on-disk project descriptor (M1 expands JSON parsing). */
struct EditorProjectSettings {
    Utf8String projectName{"Untitled"};
    Utf8String rootDirectory;
    Utf8String assetsDirectory{"assets"};
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

    /** Opens an existing folder; loads project.spark when present. */
    [[nodiscard]] bool OpenExisting(const char* projectRootUtf8) noexcept;

    [[nodiscard]] bool IsOpen() const noexcept { return isOpen; }
    [[nodiscard]] bool IsDirty() const noexcept { return dirty; }
    void MarkDirty() noexcept { dirty = true; }
    void ClearDirty() noexcept { dirty = false; }

    [[nodiscard]] bool TrySaveProjectFile() noexcept;

private:
    EditorProjectSettings settings{};
    bool isOpen = false;
    bool dirty = false;
};

}  // namespace Spark::Editor
