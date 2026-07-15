#include "spark/ecs/components/SpriteAnimatorComponent.hpp"

#include "spark/ecs/components/SpriteComponent.hpp"
#include "spark/ecs/GameObject.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

Vector4 SpriteAnimatorComponent::ComputeUniformGridUv(
        std::uint32_t columns, std::uint32_t rows, std::uint32_t linearFrame) noexcept {
    if (columns == 0 || rows == 0) {
        return {0.0F, 0.0F, 1.0F, 1.0F};
    }
    const std::uint32_t cells = columns * rows;
    linearFrame %= cells;
    const std::uint32_t col = linearFrame % columns;
    const std::uint32_t rowFromBottom = linearFrame / columns;
    const float colsF = static_cast<float>(columns);
    const float rowsF = static_cast<float>(rows);
    const float u0 = static_cast<float>(col) / colsF;
    const float u1 = static_cast<float>(col + 1U) / colsF;
    const float v0 = static_cast<float>(rowFromBottom) / rowsF;
    const float v1 = static_cast<float>(rowFromBottom + 1U) / rowsF;
    return {u0, v0, u1, v1};
}

void SpriteAnimatorComponent::SetUniformGrid(std::uint32_t c, std::uint32_t r) noexcept {
    columns = (c == 0U) ? 1U : c;
    rows = (r == 0U) ? 1U : r;
}

void SpriteAnimatorComponent::ClearClips() noexcept {
    clips.Clear();
    currentClipIndex = 0;
    timeInClipSeconds = 0.0F;
}

void SpriteAnimatorComponent::AddClip(const SpriteAnimationClip& clip) {
    clips.PushBack(clip);
}

void SpriteAnimatorComponent::SetClipIndex(std::uint32_t index) noexcept {
    if (index != currentClipIndex) {
        currentClipIndex = index;
        timeInClipSeconds = 0.0F;
    }
}

bool SpriteAnimatorComponent::IsCurrentClipFinished() const noexcept {
    if (clips.IsEmpty() || currentClipIndex >= clips.GetSize()) {
        return true;
    }
    const SpriteAnimationClip& clip = clips[currentClipIndex];
    if (clip.frameCount == 0U) {
        return true;
    }
    if (clip.loop) {
        return false;
    }
    const float fps = (clip.framesPerSecond > 1.0e-6F) ? clip.framesPerSecond : 8.0F;
    const float frameDur = 1.0F / fps;
    const float clipDur = frameDur * static_cast<float>(clip.frameCount);
    return timeInClipSeconds >= clipDur - 1.0e-4F;
}

void SpriteAnimatorComponent::OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& /*context*/) {
    SpriteComponent* sprite = owner.GetComponent<SpriteComponent>();
    if (sprite == nullptr) {
        return;
    }
    if (clips.IsEmpty() || currentClipIndex >= clips.GetSize()) {
        return;
    }
    const SpriteAnimationClip& clip = clips[currentClipIndex];
    if (clip.frameCount == 0U) {
        return;
    }
    const std::uint32_t cells = columns * rows;
    if (cells == 0U) {
        return;
    }

    const float fps = (clip.framesPerSecond > 1.0e-6F) ? clip.framesPerSecond : 8.0F;
    const float frameDur = 1.0F / fps;
    const float clipDur = frameDur * static_cast<float>(clip.frameCount);

    timeInClipSeconds += timing.deltaTimeSeconds;
    if (clip.loop) {
        if (clipDur > 1.0e-6F) {
            timeInClipSeconds = std::fmod(timeInClipSeconds, clipDur);
        }
    } else {
        timeInClipSeconds = std::min(timeInClipSeconds, std::max(clipDur - 1.0e-4F, 0.0F));
    }

    std::uint32_t localFrame = 0U;
    if (frameDur > 1.0e-8F) {
        localFrame = static_cast<std::uint32_t>(timeInClipSeconds / frameDur);
    }
    if (!clip.loop) {
        localFrame = std::min(localFrame, clip.frameCount - 1U);
    } else {
        localFrame %= clip.frameCount;
    }

    std::uint32_t linear = clip.firstFrame + localFrame;
    linear %= cells;

    sprite->SetUvRect(ComputeUniformGridUv(columns, rows, linear));
}

}  // namespace Spark
