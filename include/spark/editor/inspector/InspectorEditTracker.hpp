#pragma once

namespace Spark::Editor {

/**
 * Captures a before-snapshot on first edit and commits an undo command when editing ends.
 * Used by inspector sliders to batch drag/keyboard tweaks into one undo step.
 */
template<typename Snapshot>
class InspectorEditTracker final {
public:
    void Reset() noexcept {
        active = false;
        changed = false;
    }

    void BeginEdit(const Snapshot& current) noexcept {
        if (!active) {
            before = current;
            active = true;
            changed = false;
        }
    }

    void MarkChanged() noexcept { changed = true; }

    [[nodiscard]] bool HasPendingEdit() const noexcept { return active && changed; }

    template<typename RecordFn>
    void TryCommit(const Snapshot& current, RecordFn&& recordFn) {
        if (!active || !changed) {
            Reset();
            return;
        }
        recordFn(before, current);
        Reset();
    }

    void Cancel() noexcept { Reset(); }

private:
    Snapshot before{};
    bool active = false;
    bool changed = false;
};

}  // namespace Spark::Editor
