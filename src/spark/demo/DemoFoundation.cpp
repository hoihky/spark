#include "spark/demo/DemoFoundation.hpp"

#include "spark/ecs/components/audio/SoundCueComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/GameWorld.hpp"

#include <format>
#include <string>

namespace Spark {

namespace {

bool g_demoFpsOverlayVisible = false;

}  // namespace

void ToggleDemoFpsOverlayVisible() noexcept {
    g_demoFpsOverlayVisible = !g_demoFpsOverlayVisible;
}

bool IsDemoFpsOverlayVisible() noexcept {
    return g_demoFpsOverlayVisible;
}

void DemoRootCollection::Clear() noexcept {
    roots.Clear();
}

void DemoRootCollection::Track(GameObject* const object) noexcept {
    if (object != nullptr) {
        roots.PushBack(object);
    }
}

void DemoRootCollection::DestroyAll(GameWorld& world) noexcept {
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
}

float DemoSmoothedFps::Update(const float deltaSeconds, const std::uint32_t frameIndex) noexcept {
    const float instant = (deltaSeconds > 1.0e-6F) ? (1.0F / deltaSeconds) : 0.0F;
    if (frameIndex < 2U) {
        smoothed = instant;
    } else {
        smoothed = smoothed * 0.88F + instant * 0.12F;
    }
    return smoothed;
}

namespace DemoAudio {

void QueueCue(GameObject& actor, const SharedPtr<SoundClip>& clip, const float volume) noexcept {
    if (!clip) {
        return;
    }
    SoundCueComponent* cue = actor.GetComponent<SoundCueComponent>();
    if (cue == nullptr) {
        cue = actor.AddComponent<SoundCueComponent>();
    }
    cue->Queue(clip, volume);
}

void QueueCueAtWorld(
        GameObject& actor,
        const SharedPtr<SoundClip>& clip,
        const Vector3& worldPosition,
        const float volume,
        const float spatialBlend) noexcept {
    if (!clip) {
        return;
    }
    SoundCueComponent* cue = actor.GetComponent<SoundCueComponent>();
    if (cue == nullptr) {
        cue = actor.AddComponent<SoundCueComponent>();
    }
    cue->QueueAtWorld(clip, volume, worldPosition, spatialBlend);
}

}  // namespace DemoAudio

namespace DemoHud {

void Apply(TextOverlayComponent& overlay, const bool lightTextOnDarkBackground) noexcept {
    overlay.SetFontSizePixels(kFontSizePixels);
    overlay.SetColor(lightTextOnDarkBackground ? kLightTextColor : kDarkTextColor);
    overlay.SetAlpha(kTextAlpha);
}

}  // namespace DemoHud

void DemoFpsToggleOverlay::EnsureMounted(GameWorld& world) {
    if (object != nullptr) {
        return;
    }
    object = world.CreateGameObject();
    object->GetName() = Utf8String("ShellFpsOverlay");
    text = object->AddComponent<TextOverlayComponent>();
    DemoHud::Apply(*text);
    text->SetVisible(false);
    visible = false;
}

void DemoFpsToggleOverlay::SyncVisibilityFromGlobal() noexcept {
    visible = IsDemoFpsOverlayVisible();
    if (text != nullptr) {
        text->SetVisible(visible);
    }
}

void DemoFpsToggleOverlay::Update(const FrameTiming& timing, const int framebufferWidth) noexcept {
    if (!visible || text == nullptr) {
        return;
    }
    const float smoothed = fps.Update(timing.deltaTimeSeconds, static_cast<std::uint32_t>(timing.frameIndex));
    const std::string label = std::format("{:.0f} FPS", static_cast<double>(smoothed));
    text->SetText(Utf8String(label.c_str()));
    const float margin = DemoHud::kScreenMargin;
    const float x = framebufferWidth > 0 ? static_cast<float>(framebufferWidth) - 132.0F : 800.0F;
    text->SetScreenPosition(x, margin);
}

void DemoFpsToggleOverlay::PatchSceneRenderParams(IEngineContext& context) noexcept {
    if (!visible || text == nullptr) {
        return;
    }
    SceneRenderParams* params = nullptr;
    if (!context.TryGetMutableSceneRenderParams(params) || params == nullptr) {
        return;
    }
    if (!params->uiFont && object != nullptr) {
        const GameWorld& world = object->GetWorld();
        params->uiFont = world.GetUiFont();
        params->uiBoldFont = world.GetUiBoldFont();
    }
    ScreenTextDraw draw{};
    draw.text = text->GetText();
    draw.x = text->GetScreenX();
    draw.y = text->GetScreenY();
    draw.sizePixels = text->GetFontSizePixels();
    draw.color = text->GetColor();
    draw.alpha = text->GetAlpha();
    draw.paintOrder = params->NextUiPaintOrder();
    params->screenLateTexts.PushBack(MoveTemp(draw));
}

}  // namespace Spark
