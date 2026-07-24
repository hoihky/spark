#include "spark/scene/tilemap/TileAnimationResolve.hpp"

#include "spark/scene/tilemap/TileCell.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

std::uint16_t ResolveAnimatedTileId(
        const Tileset& tileset,
        const std::uint16_t sourceTileId,
        const float animationTimeSeconds) noexcept {
    if (sourceTileId == TileCell::kEmptyTileId) {
        return sourceTileId;
    }
    const std::uint16_t clipIndex = tileset.GetAnimationClipIndexForTile(sourceTileId);
    if (clipIndex == kNoTileAnimationClip) {
        return sourceTileId;
    }
    const Array<TileAnimationClip>& clips = tileset.GetAnimationClips();
    if (clipIndex >= clips.GetSize()) {
        return sourceTileId;
    }
    const TileAnimationClip& clip = clips[static_cast<std::size_t>(clipIndex)];
    if (clip.frames.IsEmpty()) {
        return sourceTileId;
    }

    float duration = 0.0F;
    for (std::size_t i = 0; i < clip.frames.GetSize(); ++i) {
        duration += clip.frames[i].durationSeconds > 0.0F ? clip.frames[i].durationSeconds : 0.1F;
    }
    if (duration <= 1.0e-6F) {
        return clip.frames[0].tileId;
    }

    float t = animationTimeSeconds;
    if (clip.loop) {
        t = std::fmod(t, duration);
        if (t < 0.0F) {
            t += duration;
        }
    } else {
        t = std::min(std::max(t, 0.0F), duration - 1.0e-4F);
    }

    float accum = 0.0F;
    for (std::size_t i = 0; i < clip.frames.GetSize(); ++i) {
        const float frameDur =
                clip.frames[i].durationSeconds > 0.0F ? clip.frames[i].durationSeconds : 0.1F;
        accum += frameDur;
        if (t <= accum) {
            return clip.frames[i].tileId;
        }
    }
    return clip.frames.GetLast().tileId;
}

}  // namespace Spark
