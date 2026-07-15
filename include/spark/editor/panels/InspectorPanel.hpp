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
    [[nodiscard]] Gui::Widget* GetRootWidget() noexcept override { return root_.Get(); }
    [[nodiscard]] UniquePtr<Gui::Widget> ReleaseRootWidget() { return MoveTemp(root_); }

    void OnAttach(EditorContext& ctx) override;
    void OnTick(const FrameTiming& timing, EditorContext& ctx) override;

private:
    UniquePtr<Gui::Widget> root_;
    Gui::Label* title_ = nullptr;
    Gui::Label* body_ = nullptr;
    class EditorSelection* selection_ = nullptr;
};

}  // namespace Spark::Editor
