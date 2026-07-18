#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/** Ordered waypoints in local space; consumed by <c>NavMeshAgentComponent</c>. */
class PatrolPathComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PatrolPath;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] const Array<Vector3>& GetWaypoints() const noexcept { return waypoints; }
    [[nodiscard]] Array<Vector3>& GetWaypoints() noexcept { return waypoints; }

    [[nodiscard]] bool IsLooping() const noexcept { return loop; }
    void SetLooping(const bool l) noexcept { loop = l; }

    [[nodiscard]] float GetWaitSecondsPerPoint() const noexcept { return waitSeconds; }
    void SetWaitSecondsPerPoint(const float s) noexcept { waitSeconds = s; }

private:
    Array<Vector3> waypoints{};
    bool loop = true;
    float waitSeconds = 0.0F;
};

}  // namespace Spark
