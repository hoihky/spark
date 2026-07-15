#include "spark/gui/EditorLayoutStore.hpp"

#include "spark/gui/GuiLayoutMetrics.hpp"
#include "spark/gui/GuiThemeCatalog.hpp"

#include <cstdio>
#include <cstring>

#ifndef SPARK_BUILD_ASSETS_DIR
#define SPARK_BUILD_ASSETS_DIR "."
#endif

namespace Spark::Gui {

namespace {

float gSidebarWidthPx = 600.0F;
float gSidebarSplit = 0.31F;

void LayoutFilePath(char* out, std::size_t outSz) noexcept {
    std::snprintf(out, outSz, "%s/editor_layout.ini", SPARK_BUILD_ASSETS_DIR);
}

}  // namespace

float GetSceneEditorSidebarSplit() noexcept {
    return gSidebarSplit;
}

void SetSceneEditorSidebarSplit(const float split) noexcept {
    if (split < 0.08F) {
        gSidebarSplit = 0.08F;
    } else if (split > 0.92F) {
        gSidebarSplit = 0.92F;
    } else {
        gSidebarSplit = split;
    }
}

float GetSceneEditorSidebarWidthPx() noexcept {
    return gSidebarWidthPx;
}

void SetSceneEditorSidebarWidthPx(const float widthPx) noexcept {
    gSidebarWidthPx = widthPx < 200.0F ? 200.0F : (widthPx > 1200.0F ? 1200.0F : widthPx);
}

bool TryLoadSceneEditorLayout(SceneEditorLayoutSettings& out) noexcept {
    char path[512]{};
    LayoutFilePath(path, sizeof(path));
    std::FILE* f = std::fopen(path, "r");
    if (f == nullptr) {
        out.sidebarWidthPx = gSidebarWidthPx;
        out.inspectorScrollY = 0.0F;
        return false;
    }
    char line[128]{};
    while (std::fgets(line, sizeof(line), f) != nullptr) {
        float v = 0.0F;
        if (std::strncmp(line, "sidebar_split=", 14) == 0 && std::sscanf(line + 14, "%f", &v) == 1) {
            out.sidebarSplit = v;
            out.sidebarSplitFromFile = true;
            SetSceneEditorSidebarSplit(v);
        } else if (std::strncmp(line, "sidebar_width=", 14) == 0 && std::sscanf(line + 14, "%f", &v) == 1) {
            out.sidebarWidthPx = v;
            SetSceneEditorSidebarWidthPx(v);
        } else if (std::strncmp(line, "inspector_scroll_y=", 19) == 0 && std::sscanf(line + 19, "%f", &v) == 1) {
            out.inspectorScrollY = v;
        } else if (std::strncmp(line, "ui_scale=", 9) == 0 && std::sscanf(line + 9, "%f", &v) == 1) {
            out.uiScale = v;
            SetGlobalGuiUiScale(v);
        } else if (std::strncmp(line, "gui_theme=", 10) == 0) {
            int themeId = 0;
            if (std::sscanf(line + 10, "%d", &themeId) == 1) {
                out.guiTheme = GuiThemePresetFromId(themeId);
                SetActiveGuiThemePreset(out.guiTheme);
            }
        }
    }
    std::fclose(f);
    return true;
}

bool SaveSceneEditorLayout(const SceneEditorLayoutSettings& in) noexcept {
    SetSceneEditorSidebarWidthPx(in.sidebarWidthPx);
    char path[512]{};
    LayoutFilePath(path, sizeof(path));
    std::FILE* f = std::fopen(path, "w");
    if (f == nullptr) {
        return false;
    }
    std::fprintf(f, "spark_editor_layout_v1\n");
    std::fprintf(f, "sidebar_split=%.4f\n", gSidebarSplit);
    std::fprintf(f, "sidebar_width=%.2f\n", gSidebarWidthPx);
    std::fprintf(f, "inspector_scroll_y=%.2f\n", in.inspectorScrollY);
    std::fprintf(f, "ui_scale=%.3f\n", GetGlobalGuiUiScale());
    std::fprintf(f, "gui_theme=%d\n", static_cast<int>(GetActiveGuiThemePreset()));
    std::fclose(f);
    return true;
}

}  // namespace Spark::Gui
