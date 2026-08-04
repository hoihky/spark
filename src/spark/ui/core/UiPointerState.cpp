#include "spark/ui/core/UiPointerState.hpp"

namespace Spark::Ui {

namespace {

UiPointerState gPointerState{};

}  // namespace

const UiPointerState& GetUiPointerState() noexcept {
    return gPointerState;
}

void SetUiPointerState(const UiPointerState& state) noexcept {
    gPointerState = state;
}

}  // namespace Spark::Ui
