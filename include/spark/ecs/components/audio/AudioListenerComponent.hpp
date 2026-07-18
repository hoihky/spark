#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

namespace Spark {

/**
 * Marks the owner's transform as the active audio listener (highest <c>priority</c> wins).
 * Pose is resolved each frame by <c>ProcessAudioListeners</c> before sound cues are flushed.
 */
class AudioListenerComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AudioListener;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] std::int32_t GetPriority() const noexcept { return priority; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetPriority(std::int32_t p) noexcept { priority = p; }
    void SetEnabled(bool e) noexcept { enabled = e; }

private:
    std::int32_t priority = 0;
    bool enabled = true;
};

}  // namespace Spark
