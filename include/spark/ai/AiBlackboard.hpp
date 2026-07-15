#pragma once

#include "spark/core/Array.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Shared numeric scratch space for AI modules (FSM, GOAP, steering, fuzzy) on one agent.
 * Indices are agreed by game code; the engine does not assign semantics to slots.
 */
class AiBlackboard {
public:
    AiBlackboard() {
        floats.Reserve(32);
        ints.Reserve(16);
    }

    [[nodiscard]] float GetFloat(std::size_t slot) const noexcept {
        return slot < floats.GetSize() ? floats[slot] : 0.0F;
    }
    void SetFloat(std::size_t slot, const float v) noexcept {
        EnsureFloats_(slot + 1);
        floats[slot] = v;
    }

    [[nodiscard]] int GetInt(std::size_t slot) const noexcept {
        return slot < ints.GetSize() ? ints[slot] : 0;
    }
    void SetInt(std::size_t slot, const int v) noexcept {
        EnsureInts_(slot + 1);
        ints[slot] = v;
    }

    void ClearVolatileSlots() noexcept {
        /** Optional hook: games may clear scratch each tick; default no-op. */
    }

private:
    void EnsureFloats_(const std::size_t need) noexcept {
        while (floats.GetSize() < need) {
            floats.PushBack(0.0F);
        }
    }
    void EnsureInts_(const std::size_t need) noexcept {
        while (ints.GetSize() < need) {
            ints.PushBack(0);
        }
    }

    Array<float> floats;
    Array<int> ints;
};

}  // namespace Spark
