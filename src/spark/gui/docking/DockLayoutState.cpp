#include "spark/gui/docking/DockLayoutState.hpp"

#include <cstdio>
#include <cstring>

#ifndef SPARK_BUILD_ASSETS_DIR
#define SPARK_BUILD_ASSETS_DIR "."
#endif

namespace Spark::Gui {

namespace {

void StateFilePath(char* out, const std::size_t outSz) noexcept {
    std::snprintf(out, outSz, "%s/dock_layout.ini", SPARK_BUILD_ASSETS_DIR);
}

}  // namespace

bool TryLoadDockLayoutState(DockLayoutState& out) noexcept {
    char path[512]{};
    StateFilePath(path, sizeof(path));
    std::FILE* f = std::fopen(path, "r");
    if (f == nullptr) {
        return false;
    }

    DockLayoutState state{};
    char line[128]{};
    while (std::fgets(line, sizeof(line), f) != nullptr) {
        float v = 0.0F;
        int i = 0;
        if (std::strncmp(line, "left_width=", 11) == 0 && std::sscanf(line + 11, "%f", &v) == 1) {
            state.leftWidthPx = v;
        } else if (std::strncmp(line, "right_width=", 12) == 0 && std::sscanf(line + 12, "%f", &v) == 1) {
            state.rightWidthPx = v;
        } else if (std::strncmp(line, "left_collapsed=", 15) == 0 && std::sscanf(line + 15, "%d", &i) == 1) {
            state.leftCollapsed = i != 0;
        } else if (std::strncmp(line, "right_collapsed=", 16) == 0 && std::sscanf(line + 16, "%d", &i) == 1) {
            state.rightCollapsed = i != 0;
        } else if (std::strncmp(line, "left_tab=", 9) == 0 && std::sscanf(line + 9, "%d", &i) == 1) {
            state.leftSelectedTab = i;
        }
    }
    std::fclose(f);
    out = state;
    return true;
}

bool SaveDockLayoutState(const DockLayoutState& state) noexcept {
    char path[512]{};
    StateFilePath(path, sizeof(path));
    std::FILE* f = std::fopen(path, "w");
    if (f == nullptr) {
        return false;
    }
    std::fprintf(f, "spark_dock_state_v2\n");
    std::fprintf(f, "left_width=%.2f\n", state.leftWidthPx);
    std::fprintf(f, "right_width=%.2f\n", state.rightWidthPx);
    std::fprintf(f, "left_collapsed=%d\n", state.leftCollapsed ? 1 : 0);
    std::fprintf(f, "right_collapsed=%d\n", state.rightCollapsed ? 1 : 0);
    std::fprintf(f, "left_tab=%d\n", state.leftSelectedTab);
    std::fclose(f);
    return true;
}

}  // namespace Spark::Gui
