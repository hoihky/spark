#pragma once

#include "spark/animation/locomotion/CardinalDirectionMixer.hpp"
#include "spark/animation/locomotion/DirectionalLocomotionClipCatalog.hpp"
#include "spark/animation/locomotion/ILocomotionPlaybackStrategy.hpp"

namespace Spark {

class DirectionalLocomotionStrategy final : public ILocomotionPlaybackStrategy {
public:
    DirectionalLocomotionStrategy() noexcept { cardinalMixer.SetCatalog(catalog); }

    DirectionalLocomotionClipCatalog& GetCatalog() noexcept { return catalog; }
    [[nodiscard]] const DirectionalLocomotionClipCatalog& GetCatalog() const noexcept { return catalog; }

    void BuildPlayback(
            const LocomotionPlanarSample& sample,
            const AnimatorComponent& anim,
            float walkSpeedThreshold,
            float runSpeedThreshold,
            bool wantSprint,
            LocomotionPlaybackDecision& outDecision) const noexcept override;

private:
    DirectionalLocomotionClipCatalog catalog{};
    CardinalDirectionMixer cardinalMixer{};
};

}  // namespace Spark
