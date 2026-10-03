#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class SpriteAnimatorComponent;

enum class AnimationHitbox2DShape : std::uint8_t {
    Circle = 0,
    Arc = 1,
};

/**
 * Frame-synced 2D hit window driven by a sibling <c>SpriteAnimatorComponent</c>.
 * While the active clip's local frame lies in [startFrame, endFrame], runs physics overlap
 * queries and applies damage to overlapping targets.
 */
class AnimationHitbox2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AnimationHitbox2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 215; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(set_shape)
    void SetShape(AnimationHitbox2DShape value) noexcept { shape = value; }
    SPARK_SCRIPT_BIND(get_shape)
    [[nodiscard]] AnimationHitbox2DShape GetShape() const noexcept { return shape; }

    SPARK_SCRIPT_BIND(set_clip_index)
    void SetClipIndex(std::uint32_t index) noexcept { clipIndex = index; }
    SPARK_SCRIPT_BIND(set_start_local_frame)
    void SetStartLocalFrame(std::uint32_t frame) noexcept { startLocalFrame = frame; }
    SPARK_SCRIPT_BIND(set_end_local_frame)
    void SetEndLocalFrame(std::uint32_t frame) noexcept { endLocalFrame = frame; }

    SPARK_SCRIPT_BIND(set_local_offset)
    void SetLocalOffset(const Vector2& offset) noexcept { localOffset = offset; }
    SPARK_SCRIPT_BIND(set_radius)
    void SetRadius(float radius) noexcept;
    SPARK_SCRIPT_BIND(set_arc_half_angle_radians)
    void SetArcHalfAngleRadians(float radians) noexcept;

    SPARK_SCRIPT_BIND(set_query_filter)
    void SetQueryFilter(const PhysicsQueryFilter2D& filter) noexcept { queryFilter = filter; }
    SPARK_SCRIPT_BIND(set_damage_per_hit)
    void SetDamagePerHit(float amount) noexcept { damagePerHit = amount; }

    SPARK_SCRIPT_BIND(clear_targets)
    void ClearTargets() noexcept { targets.Clear(); }
    SPARK_SCRIPT_BIND(add_target)
    void AddTarget(GameObject* target) noexcept;

    SPARK_SCRIPT_BIND(is_hit_window_active)
    [[nodiscard]] bool IsHitWindowActive() const noexcept { return hitWindowActive; }
    SPARK_SCRIPT_BIND(get_hits_this_swing)
    [[nodiscard]] std::uint32_t GetHitsThisSwing() const noexcept { return hitsThisSwing; }

private:
    void ResetSwingHits() noexcept;
    void TryOverlapHits(GameObject& owner, GameWorld& world) noexcept;
    [[nodiscard]] bool IsFrameInWindow(const SpriteAnimatorComponent& animator) const noexcept;
    [[nodiscard]] float ResolveFacingSign(const GameObject& owner) const noexcept;
    void ResolveWorldOrigin(const GameObject& owner, float& outX, float& outY) const noexcept;

    AnimationHitbox2DShape shape = AnimationHitbox2DShape::Circle;
    std::uint32_t clipIndex = 0U;
    std::uint32_t startLocalFrame = 0U;
    std::uint32_t endLocalFrame = 0U;
    Vector2 localOffset{0.0F, 0.0F};
    float radius = 0.75F;
    float arcHalfAngleRadians = 0.9F;
    float damagePerHit = 10.0F;
    PhysicsQueryFilter2D queryFilter{};
    Array<GameObject*> targets{};
    Array<GameObject*> hitThisSwing{};
    bool hitWindowActive = false;
    std::uint32_t hitsThisSwing = 0;
};

}  // namespace Spark
