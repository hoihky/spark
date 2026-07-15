#include "spark/gui/DockLayoutStore.hpp"

#include <cstdio>
#include <cstring>

#ifndef SPARK_BUILD_ASSETS_DIR
#define SPARK_BUILD_ASSETS_DIR "."
#endif

namespace Spark::Gui {

namespace {

void LayoutFilePath(char* out, const std::size_t outSz) noexcept {
    std::snprintf(out, outSz, "%s/dock_layout.ini", SPARK_BUILD_ASSETS_DIR);
}

bool ParseAxis(const char* text, DockSplitAxis& out) noexcept {
    if (std::strcmp(text, "h") == 0 || std::strcmp(text, "horizontal") == 0) {
        out = DockSplitAxis::Horizontal;
        return true;
    }
    if (std::strcmp(text, "v") == 0 || std::strcmp(text, "vertical") == 0) {
        out = DockSplitAxis::Vertical;
        return true;
    }
    return false;
}

bool ParseMeasure(const char* text, DockSplitMeasure& out) noexcept {
    if (std::strcmp(text, "frac") == 0 || std::strcmp(text, "fraction") == 0) {
        out = DockSplitMeasure::Fraction;
        return true;
    }
    if (std::strcmp(text, "px") == 0 || std::strcmp(text, "pixels") == 0) {
        out = DockSplitMeasure::LeadingPixels;
        return true;
    }
    return false;
}

}  // namespace

bool TryLoadDockLayout(DockLayoutModel& out) noexcept {
    char path[512]{};
    LayoutFilePath(path, sizeof(path));
    std::FILE* f = std::fopen(path, "r");
    if (f == nullptr) {
        return false;
    }

    DockLayoutModel model;
    int root = -1;
    char line[512]{};
    while (std::fgets(line, sizeof(line), f) != nullptr) {
        if (std::strncmp(line, "root=", 5) == 0) {
            std::sscanf(line + 5, "%d", &root);
            continue;
        }
        if (std::strncmp(line, "node=", 5) != 0) {
            continue;
        }

        int index = -1;
        char kind[16]{};
        if (std::sscanf(line + 5, "%d kind=%15s", &index, kind) < 2 || index < 0) {
            continue;
        }

        while (static_cast<int>(model.GetNodes().GetSize()) <= index) {
            (void)model.AddNode(DockNode{});
        }

        DockNode* node = model.GetNode(index);
        if (node == nullptr) {
            continue;
        }

        if (std::strcmp(kind, "leaf") == 0) {
            node->kind = DockNodeKind::Leaf;
            char panel[128]{};
            int passthrough = 0;
            if (const char* p = std::strstr(line, "panel=")) {
                std::sscanf(p + 6, "%127s", panel);
                node->panelId = Utf8String(panel);
            }
            if (const char* p = std::strstr(line, "passthrough=")) {
                std::sscanf(p + 12, "%d", &passthrough);
                node->passthroughInput = passthrough != 0;
            }
        } else if (std::strcmp(kind, "split") == 0) {
            node->kind = DockNodeKind::Split;
            char axis[16]{};
            char measure[16]{};
            float split = 0.5F;
            int child0 = -1;
            int child1 = -1;
            if (const char* p = std::strstr(line, "axis=")) {
                std::sscanf(p + 5, "%15s", axis);
                ParseAxis(axis, node->axis);
            }
            if (const char* p = std::strstr(line, "measure=")) {
                std::sscanf(p + 8, "%15s", measure);
                ParseMeasure(measure, node->measure);
            }
            if (const char* p = std::strstr(line, "split=")) {
                std::sscanf(p + 6, "%f", &split);
                node->splitValue = split;
            }
            if (const char* p = std::strstr(line, "child0=")) {
                std::sscanf(p + 7, "%d", &child0);
                node->firstChild = child0;
            }
            if (const char* p = std::strstr(line, "child1=")) {
                std::sscanf(p + 7, "%d", &child1);
                node->secondChild = child1;
            }
        } else if (std::strcmp(kind, "tabs") == 0) {
            node->kind = DockNodeKind::Tabs;
            int selected = 0;
            if (const char* p = std::strstr(line, "selected=")) {
                std::sscanf(p + 9, "%d", &selected);
                node->selectedTab = selected;
            }
            node->tabs.Clear();
            for (int tabIndex = 0; tabIndex < 32; ++tabIndex) {
                char key[16]{};
                std::snprintf(key, sizeof(key), "tab%d=", tabIndex);
                const char* p = std::strstr(line, key);
                if (p == nullptr) {
                    break;
                }
                char title[64]{};
                char panel[64]{};
                int contentNode = -1;
                if (std::sscanf(p + std::strlen(key), "%63[^:]:%63[^@]@%d", title, panel, &contentNode) >= 2) {
                    DockTabSpec tab{};
                    tab.title = Utf8String(title);
                    tab.panelId = Utf8String(panel);
                    tab.contentNodeIndex = contentNode;
                    node->tabs.PushBack(MoveTemp(tab));
                } else if (std::sscanf(p + std::strlen(key), "%63[^:]:%63s", title, panel) == 2) {
                    DockTabSpec tab{};
                    tab.title = Utf8String(title);
                    tab.panelId = Utf8String(panel);
                    node->tabs.PushBack(MoveTemp(tab));
                }
            }
        }
    }
    std::fclose(f);

    if (root < 0 || model.GetNode(root) == nullptr) {
        return false;
    }
    model.SetRoot(root);
    out = MoveTemp(model);
    return true;
}

bool SaveDockLayout(const DockLayoutModel& model) noexcept {
    char path[512]{};
    LayoutFilePath(path, sizeof(path));
    std::FILE* f = std::fopen(path, "w");
    if (f == nullptr) {
        return false;
    }

    std::fprintf(f, "spark_dock_layout_v1\n");
    std::fprintf(f, "root=%d\n", model.GetRoot());

    const Array<DockNode>& nodes = model.GetNodes();
    for (std::size_t i = 0; i < nodes.GetSize(); ++i) {
        const DockNode& node = nodes[i];
        const int index = static_cast<int>(i);
        if (node.kind == DockNodeKind::Leaf) {
            std::fprintf(f,
                    "node=%d kind=leaf panel=%s passthrough=%d\n",
                    index,
                    node.panelId.CStr(),
                    node.passthroughInput ? 1 : 0);
        } else if (node.kind == DockNodeKind::Split) {
            const char* axis = node.axis == DockSplitAxis::Horizontal ? "h" : "v";
            const char* measure = node.measure == DockSplitMeasure::Fraction ? "frac" : "px";
            std::fprintf(f,
                    "node=%d kind=split axis=%s measure=%s split=%.4f child0=%d child1=%d\n",
                    index,
                    axis,
                    measure,
                    node.splitValue,
                    node.firstChild,
                    node.secondChild);
        } else if (node.kind == DockNodeKind::Tabs) {
            std::fprintf(f, "node=%d kind=tabs selected=%d", index, node.selectedTab);
            for (std::size_t t = 0; t < node.tabs.GetSize(); ++t) {
                const DockTabSpec& tab = node.tabs[t];
                if (tab.contentNodeIndex >= 0) {
                    std::fprintf(f,
                            " tab%zu=%s:%s@%d",
                            t,
                            tab.title.CStr(),
                            tab.panelId.CStr(),
                            tab.contentNodeIndex);
                } else {
                    std::fprintf(f, " tab%zu=%s:%s", t, tab.title.CStr(), tab.panelId.CStr());
                }
            }
            std::fprintf(f, "\n");
        }
    }

    std::fclose(f);
    return true;
}

}  // namespace Spark::Gui
