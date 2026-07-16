#pragma once

#include "spark/editor/IEditorPanel.hpp"
#include "spark/gui/controls/Label.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark::Editor {

class InspectorPanel final : public IEditorPanel {
public:
    InspectorPanel();
    ~InspectorPanel() override = default;

    [[nodiscard]] Utf8String GetPanelId() const override { return Utf8String("inspector"); }
    [[nodiscard]] Utf8String GetDisplayName() const override { return Utf8String("Inspector"); }
    [[nodiscard]] Gui::Widget* GetRootWidget() noexcept override { return root.Get(); }
    [[nodiscard]] UniquePtr<Gui::Widget> ReleaseRootWidget() { return MoveTemp(root); }

    void OnAttach(EditorContext& ctx) override;
    void OnTick(const FrameTiming& timing, EditorContext& ctx) override;

private:
    UniquePtr<Gui::Widget> root;
    Gui::Label* title = nullptr;
    Gui::Label* body = nullptr;
    class EditorSelection* selection = nullptr;
};

}  // namespace Spark::Editor
