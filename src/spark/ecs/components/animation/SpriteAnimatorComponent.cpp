#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"

#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/Texture2D.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

Vector4 SpriteAnimatorComponent::ComputeUniformGridUv(
        const std::uint32_t columns,
        const std::uint32_t rows,
        const std::uint32_t linearFrame,
        const std::uint32_t atlasPixelWidth,
        const std::uint32_t atlasPixelHeight) noexcept {
    if (columns == 0 || rows == 0) {
        return {0.0F, 0.0F, 1.0F, 1.0F};
    }
    const std::uint32_t cells = columns * rows;
    const std::uint32_t frame = linearFrame % cells;
    const std::uint32_t col = frame % columns;
    const std::uint32_t rowFromBottom = frame / columns;
    const float colsF = static_cast<float>(columns);
    const float rowsF = static_cast<float>(rows);
    float u0 = static_cast<float>(col) / colsF;
    float u1 = static_cast<float>(col + 1U) / colsF;
    float v0 = static_cast<float>(rowFromBottom) / rowsF;
    float v1 = static_cast<float>(rowFromBottom + 1U) / rowsF;
    if (atlasPixelWidth > 0U && atlasPixelHeight > 0U) {
        const float halfU = 0.5F / static_cast<float>(atlasPixelWidth);
        const float halfV = 0.5F / static_cast<float>(atlasPixelHeight);
        u0 += halfU;
        u1 -= halfU;
        v0 += halfV;
        v1 -= halfV;
    }
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

    const SharedPtr<Texture2D>& atlas = sprite->GetTexture();
    const std::uint32_t atlasW = atlas ? atlas->GetWidth() : 0U;
    const std::uint32_t atlasH = atlas ? atlas->GetHeight() : 0U;
    sprite->SetUvRect(ComputeUniformGridUv(columns, rows, linear, atlasW, atlasH));
}

}  // namespace Spark
