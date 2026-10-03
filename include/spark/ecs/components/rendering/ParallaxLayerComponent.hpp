#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"
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

    SPARK_SCRIPT_BIND(set_camera_reference)
    void SetCameraReference(GameObject* camera) noexcept { cameraReference = camera; }
    SPARK_SCRIPT_BIND(get_camera_reference)
    [[nodiscard]] GameObject* GetCameraReference() const noexcept { return cameraReference; }

    SPARK_SCRIPT_BIND(set_anchor_world)
    void SetAnchorWorld(const Vector3& anchor) noexcept { anchorWorld = anchor; }

    SPARK_SCRIPT_BIND(get_anchor_world)
    [[nodiscard]] const Vector3& GetAnchorWorld() const noexcept { return anchorWorld; }

    SPARK_SCRIPT_BIND(set_factor_xy)
    SPARK_SCRIPT_BIND(set_factor_x_y)
    void SetFactorXY(float factorXIn, float factorYIn) noexcept
    {
        SetFactorX(factorXIn);
        SetFactorY(factorYIn);
    }

    SPARK_SCRIPT_BIND(get_factor_xy)
    SPARK_SCRIPT_BIND(get_factor_x_y)
    void GetFactorXY(float* outFactorX, float* outFactorY) const noexcept
    {
        if (outFactorX != nullptr) {
            *outFactorX = factorX;
        }
        if (outFactorY != nullptr) {
            *outFactorY = factorY;
        }
    }

    SPARK_SCRIPT_BIND(set_rest_offset)
    void SetRestOffset(const Vector3& offset) noexcept { restOffset = offset; }
    SPARK_SCRIPT_BIND(get_rest_offset)
    [[nodiscard]] const Vector3& GetRestOffset() const noexcept { return restOffset; }

    SPARK_SCRIPT_BIND(set_factor_x)
    void SetFactorX(const float factor) noexcept { factorX = factor; }
    SPARK_SCRIPT_BIND(set_factor_y)
    void SetFactorY(const float factor) noexcept { factorY = factor; }
    SPARK_SCRIPT_BIND(get_factor_x)
    [[nodiscard]] float GetFactorX() const noexcept { return factorX; }
    SPARK_SCRIPT_BIND(get_factor_y)
    [[nodiscard]] float GetFactorY() const noexcept { return factorY; }

    SPARK_SCRIPT_BIND(set_axis_mode)
    void SetAxisMode(const ParallaxAxisMode mode) noexcept { axisMode = mode; }

    SPARK_SCRIPT_BIND(get_axis_mode)
    [[nodiscard]] ParallaxAxisMode GetAxisMode() const noexcept { return axisMode; }

    SPARK_SCRIPT_BIND(set_drift)
    void SetDrift(
            ParallaxDriftMode mode,
            float amplitude,
            float frequencyHz,
            float phase) noexcept
    {
        SetDriftMode(mode);
        SetDriftAmplitude(amplitude);
        SetDriftFrequencyHz(frequencyHz);
        SetDriftPhase(phase);
    }

    SPARK_SCRIPT_BIND(set_drift_mode)
    void SetDriftMode(const ParallaxDriftMode mode) noexcept { driftMode = mode; }
    SPARK_SCRIPT_BIND(set_drift_amplitude)
    void SetDriftAmplitude(const float value) noexcept { driftAmplitude = value; }
    SPARK_SCRIPT_BIND(set_drift_frequency_hz)
    void SetDriftFrequencyHz(const float value) noexcept { driftFrequencyHz = value; }
    SPARK_SCRIPT_BIND(set_drift_phase)
    void SetDriftPhase(const float value) noexcept { driftPhase = value; }

    /** Manual tick for demos that bypass <c>UpdateGameObjects</c>. */
    SPARK_SCRIPT_BIND(tick)
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
