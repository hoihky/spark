#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameObject;
class IEngineContext;

/**
 * Advances global tile animation time for the sibling <c>TilemapComponent</c>.
 * The render path resolves animated atlas ids via <c>ResolveAnimatedTileId</c> at submit time.
 */
class TilemapTileAnimatorComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TilemapTileAnimator;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] float GetAnimationTimeSeconds() const noexcept { return animationTimeSeconds; }
    void SetAnimationTimeSeconds(const float seconds) noexcept { animationTimeSeconds = seconds; }
    void ResetAnimationTime() noexcept { animationTimeSeconds = 0.0F; }

    [[nodiscard]] bool IsPlaying() const noexcept { return playing; }
    void SetPlaying(const bool play) noexcept { playing = play; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    float animationTimeSeconds = 0.0F;
    bool playing = true;
};

}  // namespace Spark
