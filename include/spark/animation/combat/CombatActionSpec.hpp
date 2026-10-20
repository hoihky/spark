#pragma once

#include "spark/animation/combat/CombatActionKind.hpp"
#include "spark/animation/combat/CombatActionTimeline.hpp"
#include "spark/animation/LocomotionClipSet.hpp"

#include <cstdint>

namespace Spark {

class CombatActionSpec final {
public:
    void SetKind(const CombatActionKind kindIn) noexcept { kind = kindIn; }
    void SetClipIndex(const std::uint32_t clip) noexcept { clipIndex = clip; }
    void SetUpperBodyOverlay(const bool value) noexcept { upperBodyOverlay = value; }
    void SetUseAttackRootMotion(const bool value) noexcept { useAttackRootMotion = value; }
    void SetInterruptPriority(const int value) noexcept { interruptPriority = value; }
    void SetTimeline(const CombatActionTimeline& timelineIn) noexcept { timeline = timelineIn; }

    [[nodiscard]] CombatActionKind GetKind() const noexcept { return kind; }
    [[nodiscard]] std::uint32_t GetClipIndex() const noexcept { return clipIndex; }
    [[nodiscard]] bool UsesUpperBodyOverlay() const noexcept { return upperBodyOverlay; }
    [[nodiscard]] bool UsesAttackRootMotion() const noexcept { return useAttackRootMotion; }
    [[nodiscard]] int GetInterruptPriority() const noexcept { return interruptPriority; }
    [[nodiscard]] const CombatActionTimeline& GetTimeline() const noexcept { return timeline; }
    [[nodiscard]] bool IsConfigured() const noexcept { return clipIndex != kInvalidAnimClipIndex; }

private:
    CombatActionKind kind{};
    std::uint32_t clipIndex = kInvalidAnimClipIndex;
    bool upperBodyOverlay = true;
    bool useAttackRootMotion = false;
    int interruptPriority = 0;
    CombatActionTimeline timeline{};
};

}  // namespace Spark
