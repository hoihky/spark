#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector4.hpp"

#include <cstdint>

namespace Spark {

/**
 * One clip in a uniform grid atlas: frames are linear indices (row-major from bottom-left cell).
 * The sibling SpriteComponent's uvRect is overwritten each OnUpdate.
 */
struct SpriteAnimationClip {
    std::uint32_t firstFrame = 0;
    std::uint32_t frameCount = 1;
    float framesPerSecond = 8.0F;
    bool loop = true;
};

/**
 * Advances 2D sprite frame time and updates SpriteComponent UV for a uniform grid atlas (cols × rows).
 */
class SpriteAnimatorComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SpriteAnimator;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override {
        return ComponentUpdatePriority::AnimatorPlayback;
    }

    SpriteAnimatorComponent() = default;

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    /** Each cell is 1/(cols) × 1/(rows); frame 0 is bottom-left, increasing along +X then +Y. */
    void SetUniformGrid(std::uint32_t columns, std::uint32_t rows) noexcept;

    void ClearClips() noexcept;
    void AddClip(const SpriteAnimationClip& clip);

    void SetClipIndex(std::uint32_t index) noexcept;
    [[nodiscard]] std::uint32_t GetClipIndex() const noexcept { return currentClipIndex; }
    [[nodiscard]] std::uint32_t GetClipCount() const noexcept {
        return static_cast<std::uint32_t>(clips.GetSize());
    }

    /** For the current clip: when <c>loop == false</c>, true once playback reaches the last frame. */
    [[nodiscard]] bool IsCurrentClipFinished() const noexcept;

    [[nodiscard]] static Vector4 ComputeUniformGridUv(
            std::uint32_t columns,
            std::uint32_t rows,
            std::uint32_t linearFrame,
            std::uint32_t atlasPixelWidth = 0,
            std::uint32_t atlasPixelHeight = 0) noexcept;

private:
    std::uint32_t columns = 1;
    std::uint32_t rows = 1;
    Array<SpriteAnimationClip> clips{};
    std::uint32_t currentClipIndex = 0;
    float timeInClipSeconds = 0.0F;
};

}  // namespace Spark
