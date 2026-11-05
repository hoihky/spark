#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"

#include <cstdint>

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

    void SetShape(AnimationHitbox2DShape value) noexcept { shape = value; }
    [[nodiscard]] AnimationHitbox2DShape GetShape() const noexcept { return shape; }

    void SetClipIndex(std::uint32_t index) noexcept { clipIndex = index; }
    void SetStartLocalFrame(std::uint32_t frame) noexcept { startLocalFrame = frame; }
    void SetEndLocalFrame(std::uint32_t frame) noexcept { endLocalFrame = frame; }

    void SetLocalOffset(const Vector2& offset) noexcept { localOffset = offset; }
    void SetRadius(float radius) noexcept;
    void SetArcHalfAngleRadians(float radians) noexcept;

    void SetQueryFilter(const PhysicsQueryFilter2D& filter) noexcept { queryFilter = filter; }
    void SetDamagePerHit(float amount) noexcept { damagePerHit = amount; }

    void ClearTargets() noexcept { targets.Clear(); }
    void AddTarget(GameObject* target) noexcept;

    [[nodiscard]] bool IsHitWindowActive() const noexcept { return hitWindowActive; }
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
