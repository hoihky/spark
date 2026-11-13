#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameWorld;

/** One rigidbody row in a recorded physics frame. */
struct PhysicsRigidbodySnapshot3D {
    std::uint64_t objectId = 0;
    Vector3 translation{Vector3::Zero};
    Vector3 velocity{Vector3::Zero};
    Vector3 angularVelocity{Vector3::Zero};
};

/** Ring buffer of physics snapshots for replay / QA (engine hook only). */
class PhysicsReplayRingBuffer3D {
public:
    void SetCapacity(std::size_t frameCapacity) noexcept;
    [[nodiscard]] std::size_t GetCapacity() const noexcept { return capacity; }
    [[nodiscard]] std::size_t GetRecordedFrameCount() const noexcept { return frames.GetSize(); }

    void Clear() noexcept;
    void CaptureFrame(GameWorld& world);
    [[nodiscard]] bool RestoreFrame(std::size_t frameIndexFromLatest, GameWorld& world) const noexcept;

private:
    std::size_t capacity = 64;
    Array<Array<PhysicsRigidbodySnapshot3D>> frames{};
};

/** Controls whether live simulation advances and whether frames are recorded. */
class PhysicsSimulationDomain3D {
public:
    enum class AdvanceMode : std::uint8_t {
        Live = 0,
        Paused = 1,
        ReplayPlayback = 2,
    };

    void SetAdvanceMode(AdvanceMode modeIn) noexcept { mode = modeIn; }
    [[nodiscard]] AdvanceMode GetAdvanceMode() const noexcept { return mode; }
    [[nodiscard]] bool ShouldAdvanceLiveSimulation() const noexcept { return mode == AdvanceMode::Live; }

    void SetRecordingEnabled(bool enabledIn) noexcept { recordingEnabled = enabledIn; }
    [[nodiscard]] bool IsRecordingEnabled() const noexcept { return recordingEnabled; }

    [[nodiscard]] PhysicsReplayRingBuffer3D& GetReplayBuffer() noexcept { return replayBuffer; }
    [[nodiscard]] const PhysicsReplayRingBuffer3D& GetReplayBuffer() const noexcept { return replayBuffer; }

    void OnLiveStepFinished(GameWorld& world);

    /** Applies a stored frame without running the solver (replay scrub). */
    [[nodiscard]] bool PlaybackFrameFromLatest(std::size_t framesBack, GameWorld& world) const noexcept;

private:
    AdvanceMode mode = AdvanceMode::Live;
    bool recordingEnabled = false;
    PhysicsReplayRingBuffer3D replayBuffer{};
};

}  // namespace Spark
