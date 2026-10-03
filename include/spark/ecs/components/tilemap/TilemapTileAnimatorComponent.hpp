#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class IEngineContext;

/**
 * Advances global tile animation time for the sibling <c>TilemapComponent</c>.
 * The render path resolves animated atlas ids via <c>TileAnimationResolver</c> at submit time.
 */
class TilemapTileAnimatorComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TilemapTileAnimator;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_animation_time_seconds)
    [[nodiscard]] float GetAnimationTimeSeconds() const noexcept { return animationTimeSeconds; }
    SPARK_SCRIPT_BIND(set_animation_time_seconds)
    void SetAnimationTimeSeconds(const float seconds) noexcept { animationTimeSeconds = seconds; }
    SPARK_SCRIPT_BIND(reset_animation_time)
    void ResetAnimationTime() noexcept { animationTimeSeconds = 0.0F; }

    SPARK_SCRIPT_BIND(is_playing)
    [[nodiscard]] bool IsPlaying() const noexcept { return playing; }
    SPARK_SCRIPT_BIND(set_playing)
    void SetPlaying(const bool play) noexcept { playing = play; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    float animationTimeSeconds = 0.0F;
    bool playing = true;
};

}  // namespace Spark
