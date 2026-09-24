#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class GameObject;

/** Which world axes participate in camera-relative parallax scrolling. */
enum class ParallaxAxisMode : std::uint8_t {
    Horizontal = 0,
    Both = 1,
};

/** Optional procedural drift layered on top of parallax (e.g. slow cloud sway). */
enum class ParallaxDriftMode : std::uint8_t {
    None = 0,
    SineHorizontal = 1,
};

/**
 * Scrolls a background layer slower/faster than the camera using a parallax factor.
 * Attach to distant sprites; point <c>cameraReference</c> at the active camera rig object.
 *
 * Runs at priority 290 (before <c>ScreenShakeComponent</c> and <c>Camera2DRigComponent</c>).
 */
class ParallaxLayerComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::ParallaxLayer;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 290; }

    void OnAttach(GameObject& owner) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    void SetCameraReference(GameObject* camera) noexcept { cameraReference = camera; }
    [[nodiscard]] GameObject* GetCameraReference() const noexcept { return cameraReference; }

    void SetAnchorWorld(const Vector3& anchor) noexcept { anchorWorld = anchor; }
    [[nodiscard]] const Vector3& GetAnchorWorld() const noexcept { return anchorWorld; }

    void SetRestOffset(const Vector3& offset) noexcept { restOffset = offset; }
    [[nodiscard]] const Vector3& GetRestOffset() const noexcept { return restOffset; }

    void SetFactorX(const float factor) noexcept { factorX = factor; }
    void SetFactorY(const float factor) noexcept { factorY = factor; }
    [[nodiscard]] float GetFactorX() const noexcept { return factorX; }
    [[nodiscard]] float GetFactorY() const noexcept { return factorY; }

    void SetAxisMode(const ParallaxAxisMode mode) noexcept { axisMode = mode; }
    [[nodiscard]] ParallaxAxisMode GetAxisMode() const noexcept { return axisMode; }

    void SetDriftMode(const ParallaxDriftMode mode) noexcept { driftMode = mode; }
    void SetDriftAmplitude(const float value) noexcept { driftAmplitude = value; }
    void SetDriftFrequencyHz(const float value) noexcept { driftFrequencyHz = value; }
    void SetDriftPhase(const float value) noexcept { driftPhase = value; }

    /** Manual tick for demos that bypass <c>UpdateGameObjects</c>. */
    static void Tick(ParallaxLayerComponent& layer, GameObject& owner, float deltaSeconds, float sceneTime) noexcept;

private:
    GameObject* cameraReference = nullptr;
    Vector3 anchorWorld{0.0F, 0.0F, 0.0F};
    Vector3 restOffset{0.0F, 0.0F, 0.0F};
    float factorX = 0.1F;
    float factorY = 0.0F;
    ParallaxAxisMode axisMode = ParallaxAxisMode::Horizontal;
    ParallaxDriftMode driftMode = ParallaxDriftMode::None;
    float driftAmplitude = 0.0F;
    float driftFrequencyHz = 0.2F;
    float driftPhase = 0.0F;
    float driftTime = 0.0F;
    bool restCaptured = false;
};

}  // namespace Spark
