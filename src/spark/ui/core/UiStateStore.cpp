#include "spark/ui/core/IUiStateStore.hpp"

namespace Spark::Ui {

bool& UiStateStore::BoolState(const UiElementId& id, const bool defaultValue) {
    if (bool* existing = boolStates.Find(id)) {
        return *existing;
    }
    boolStates.Add(id, defaultValue);
    return *boolStates.Find(id);
}

float& UiStateStore::FloatState(const UiElementId& id, const float defaultValue) {
    if (float* existing = floatStates.Find(id)) {
        return *existing;
    }
    floatStates.Add(id, defaultValue);
    return *floatStates.Find(id);
}

int& UiStateStore::IntState(const UiElementId& id, const int defaultValue) {
    if (int* existing = intStates.Find(id)) {
        return *existing;
    }
    intStates.Add(id, defaultValue);
    return *intStates.Find(id);
}

void UiStateStore::ClearFrameTransient() noexcept {
    // Phase 1+: clear per-frame click flags; persistent maps kept for now.
}

}  // namespace Spark::Ui
