#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

namespace Spark {

/** How a billboard aligns its owner's rotation toward the active camera. */
enum class BillboardMode : std::uint8_t {
    /** Full camera-facing (right/up from camera basis). */
    CameraFacing = 0,
    /** Rotate around world +Y only (common for trees / pickups in 3D). */
    YAxisLocked = 1,
};

/**
 * Orients the owner's <c>TransformComponent</c> each frame toward the resolved main camera.
 * Runs before most gameplay (priority 50) so downstream systems see the updated pose.
 */
class BillboardComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Billboard;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 50; }

    [[nodiscard]] BillboardMode GetMode() const noexcept { return mode; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetMode(BillboardMode m) noexcept { mode = m; }
    void SetEnabled(bool e) noexcept { enabled = e; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    BillboardMode mode = BillboardMode::CameraFacing;
    bool enabled = true;
};

}  // namespace Spark
