#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scene/ScenePartitionKind.hpp"

namespace Spark {

/**
 * Optional ECS policy: attach to any GameObject (often a singleton "Game" root). Call
 * Scene::ApplySpatialPolicyFromFirstMatchingObject() each frame or after edits so the Scene uses
 * this partition mode for ForEach*InViewFrustum helpers.
 */
class SceneSpatialPolicyComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SceneSpatialPolicy;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SceneSpatialPolicyComponent(ScenePartitionKind initial = ScenePartitionKind::None) noexcept
        : partition(initial) {}

    [[nodiscard]] ScenePartitionKind GetPartitionKind() const noexcept { return partition; }
    void SetPartitionKind(ScenePartitionKind k) noexcept { partition = k; }

private:
    ScenePartitionKind partition;
};

}  // namespace Spark
