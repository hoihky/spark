#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GameWorld.hpp"

#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/SceneRenderParams.hpp"

#include <cstdint>

namespace Spark {

class IEngineContext;

class GameObject;
class SoundClip;
class SoundCueComponent;
class TextOverlayComponent;

/** Tracks spawned objects and destroys them in bulk (RAII-style teardown for demos). */
class DemoRootCollection {
public:
    void Clear() noexcept;
    void Track(GameObject* object) noexcept;
    void DestroyAll(GameWorld& world) noexcept;

    [[nodiscard]] Array<GameObject*>& GetRoots() noexcept { return roots; }
    [[nodiscard]] const Array<GameObject*>& GetRoots() const noexcept { return roots; }

private:
    Array<GameObject*> roots{};
};

/** Exponential moving average FPS for HUD overlays. */
class DemoSmoothedFps {
public:
    [[nodiscard]] float Update(const float deltaSeconds, const std::uint32_t frameIndex) noexcept;

private:
    float smoothed = 0.0F;
};

namespace DemoAudio {

/** Routes one-shots through <c>SoundCueComponent</c> (processed by <c>ProcessSoundCues</c>). */
void QueueCue(
        GameObject& actor,
        const SharedPtr<SoundClip>& clip,
        float volume = 1.0F) noexcept;

void QueueCueAtWorld(
        GameObject& actor,
        const SharedPtr<SoundClip>& clip,
        const Vector3& worldPosition,
        float volume = 1.0F,
        float spatialBlend = 1.0F) noexcept;

}  // namespace DemoAudio

/** Shared on-screen help / FPS overlay styling for shell demos. */
namespace DemoHud {

constexpr float kFontSizePixels = 28.0F;
constexpr float kScreenMargin = 14.0F;

/** Bright text for 3D / darker scene backgrounds. */
constexpr Vector3 kLightTextColor{1.0F, 1.0F, 1.0F};
/** Dark text for bright 2D playfields and tilemaps. */
constexpr Vector3 kDarkTextColor{0.05F, 0.08F, 0.13F};
constexpr float kTextAlpha = 1.0F;

void Apply(TextOverlayComponent& overlay, bool lightTextOnDarkBackground = true) noexcept;

}  // namespace DemoHud

/** Global F3 toggle (handled in <c>Engine</c> before game input). */
void ToggleDemoFpsOverlayVisible() noexcept;
[[nodiscard]] bool IsDemoFpsOverlayVisible() noexcept;

/** Shell-wide FPS readout (top-right); toggled with F3 in SparkDemo. */
class DemoFpsToggleOverlay {
public:
    void EnsureMounted(GameWorld& world);
    void SyncVisibilityFromGlobal() noexcept;
    void Update(const FrameTiming& timing, int framebufferWidth) noexcept;
    void PatchSceneRenderParams(IEngineContext& context) noexcept;

private:
    GameObject* object = nullptr;
    TextOverlayComponent* text = nullptr;
    bool visible = false;
    DemoSmoothedFps fps{};
};

}  // namespace Spark
