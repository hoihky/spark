#pragma once

#include <cstdint>

namespace Spark {

class DirectionalLocomotionClipCatalog;

/** Blends between cardinal locomotion clips from a signed body-relative move angle. */
class CardinalDirectionMixer final {
public:
    void SetCatalog(const DirectionalLocomotionClipCatalog& catalogIn) noexcept { catalog = &catalogIn; }

    void Evaluate(
            float signedMoveAngleRadians,
            float planarSpeed,
            float walkSpeedThreshold,
            float runSpeedThreshold,
            bool wantSprint,
            std::uint32_t& outClipA,
            std::uint32_t& outClipB,
            float& outBlend01) const noexcept;

private:
    [[nodiscard]] std::uint32_t PickGaitClip(
            std::uint32_t walkClip,
            std::uint32_t runClip,
            float planarSpeed,
            float walkSpeedThreshold,
            float runSpeedThreshold,
            bool wantSprint) const noexcept;

    const DirectionalLocomotionClipCatalog* catalog = nullptr;
};

}  // namespace Spark
