#pragma once

#include "spark/animation/combat/CombatActionKind.hpp"
#include "spark/animation/combat/CombatGraphRunner.hpp"
#include "spark/animation/combat/CombatPhaseKind.hpp"

#include <cstdint>

namespace Spark {

class AnimationLayerStackComponent;
class AnimatorComponent;
class GameObject;
class Character3DAnimFsmComponent;

/** Drives upper-body combat overlays using <c>CombatGraphRunner</c> cancel rules. */
class Character3DCombatGraphDriver final {
public:
    explicit Character3DCombatGraphDriver(Character3DAnimFsmComponent& ownerIn) noexcept : owner(ownerIn) {}

    void RebuildActionSpecs() noexcept;

    void ProcessFrame(AnimatorComponent& anim, GameObject& visual, CombatActionKind& pendingAction) noexcept;

    [[nodiscard]] bool IsOverlayPresenting() const noexcept;
    [[nodiscard]] CombatPhaseKind GetActivePhase() const noexcept { return graph.GetActivePhase(); }
    [[nodiscard]] CombatActionKind GetActiveAction() const noexcept { return graph.GetActiveAction(); }

private:
    [[nodiscard]] bool ClipValid_(const AnimatorComponent& anim, std::uint32_t clip) const noexcept;
    void StartOverlay_(
            AnimationLayerStackComponent& stack,
            AnimatorComponent& anim,
            GameObject& visual,
            const CombatActionSpec& spec) noexcept;
    void EndRootMotion_(GameObject& visual) noexcept;

    Character3DAnimFsmComponent& owner;
    CombatGraphRunner graph{};
    CombatActionKind presentingAction{};
};

}  // namespace Spark
