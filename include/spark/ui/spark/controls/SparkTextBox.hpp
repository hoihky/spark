#pragma once

#include "spark/ui/controls/IUiControls.hpp"
#include "spark/ui/core/UiElementBase.hpp"
#include "spark/ui/factory/ControlDesc.hpp"

namespace Spark {

class IInput;

namespace Ui {

class SparkTextBox final : public ITextBox, public UiElementBase {
public:
    explicit SparkTextBox(const TextFieldDesc& desc);

    void SetText(Utf8String textIn) override;
    [[nodiscard]] Utf8StringView GetText() const noexcept override { return draft; }
    void SetOnCommit(UiVoidCallback handler) override { onCommit = handler; }
    [[nodiscard]] bool IsEditing() const noexcept override { return focused; }

    [[nodiscard]] bool WantsKeyboardFocus() const override { return true; }
    void OnFocusGained() override;
    void OnFocusLost() override;
    void ProcessKeyInput(IInput& input) override;

protected:
    void DoMeasure(const UiMeasureConstraints& constraints, UiSize& outDesired) override;
    void DoPaint(IUiRenderer& renderer) override;

private:
    void CommitDraft();
    void AppendCodepoint(std::uint32_t codepoint);
    void DeleteBeforeCaret();
    void DeleteAfterCaret();
    void MoveCaretLeft();
    void MoveCaretRight();
    void MoveCaretHome();
    void MoveCaretEnd();

    Utf8String label{};
    Utf8String committed{};
    Utf8String draft{};
    std::size_t caretByteIndex = 0;
    UiVoidCallback onCommit{};
    bool focused = false;
};

}  // namespace Ui
}  // namespace Spark
