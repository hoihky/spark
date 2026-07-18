#pragma once

#include "spark/audio/SoundClip.hpp"
#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

class SoundEngine;

struct SoundPlayRequest {
    SharedPtr<SoundClip> clip{};
    float volume = 1.0F;
    Vector3 worldPosition{0.0F, 0.0F, 0.0F};
    float spatialBlend = 0.0F;
    float minDistance = 1.0F;
    float maxDistance = 48.0F;
};

/**
 * Queue one-shots from gameplay; <c>ProcessSoundCues</c> drains into the global <c>SoundEngine</c> each frame
 * (Dependency Inversion: components queue, subsystem submits).
 */
class SoundCueComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SoundCue;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void Queue(const SharedPtr<SoundClip>& clip, float volume = 1.0F);

    /** Queues a spatial cue using the owner's world position at flush time. */
    void QueueAtWorld(
            const SharedPtr<SoundClip>& clip,
            float volume,
            const Vector3& worldPosition,
            float spatialBlend = 1.0F,
            float minDistance = 1.0F,
            float maxDistance = 48.0F);

    /** Plays all pending cues and clears the queue (called by audio subsystem). */
    void FlushTo(SoundEngine* engine) noexcept;

private:
    Array<SoundPlayRequest> pending{};
};

}  // namespace Spark
