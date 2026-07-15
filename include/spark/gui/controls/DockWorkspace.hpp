#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/core/Utility.hpp"
#include "spark/gui/DockLayoutModel.hpp"
#include "spark/gui/Widget.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <functional>

namespace Spark::Gui {

/**
 * Builds and hosts a retained widget tree from a <c>DockLayoutModel</c>.
 *
 * Panel widgets are owned by the workspace and referenced by id (e.g. <c>"hierarchy"</c>).
 * Split and tab nodes map to <c>Splitter</c> / <c>TabControl</c> primitives. Leaf nodes with
 * <c>passthroughInput</c> forward pointer hits to the scene behind the UI.
 */
class DockWorkspace final : public Widget {
public:
    struct PanelSlot {
        Utf8String id;
        UniquePtr<Widget> content;
        bool useChrome = true;
    };

    void SetLayout(DockLayoutModel model);
    [[nodiscard]] const DockLayoutModel& GetLayout() const noexcept { return model_; }

    void SetPanel(Utf8String panelId, UniquePtr<Widget> content, bool useChrome = true);
    [[nodiscard]] Widget* GetPanelWidget(const Utf8String& panelId) noexcept;

    void SetOnLayoutChanged(std::function<void(const DockLayoutModel&)> fn) {
        onLayoutChanged_ = Spark::MoveTemp(fn);
    }

    void Arrange(const Rect& r) override;
    void Paint(GuiPaintContext& ctx) const override;
    [[nodiscard]] Widget* FindDeepestHover(float x, float y) override;

private:
    struct PanelSlotEntry {
        Utf8String id;
        UniquePtr<Widget> content;
        bool useChrome = true;
    };

    void MarkLayoutDirty() noexcept { layoutDirty_ = true; }
    void RebuildIfNeeded();
    UniquePtr<Widget> BuildNode(int nodeIndex);
    UniquePtr<Widget> BuildLeaf(const DockNode& node);
    UniquePtr<Widget> BuildTabs(const DockNode& node, int nodeIndex);
    UniquePtr<Widget> BuildSplit(const DockNode& node, int nodeIndex);
    [[nodiscard]] Widget* FindPanelContent(const Utf8String& panelId) noexcept;
    void NotifyLayoutChanged(bool committed);

    DockLayoutModel model_{};
    Array<PanelSlotEntry> panels_{};
    UniquePtr<Widget> builtRoot_{};
    bool layoutDirty_ = true;
    std::function<void(const DockLayoutModel&)> onLayoutChanged_{};
};

}  // namespace Spark::Gui
