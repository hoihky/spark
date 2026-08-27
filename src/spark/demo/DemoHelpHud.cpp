#include "spark/demo/DemoHelpHud.hpp"

#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/text/Font.hpp"
#include "spark/ui/core/UiPaintContext.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>

namespace Spark {

namespace {

bool g_demoHelpHudVisible = false;
DemoHelpHud* g_activeHelpHud = nullptr;

constexpr float kWrapWidthFraction = 0.42F;
constexpr float kMinWrapWidthPixels = 280.0F;
constexpr float kMaxWrapWidthPixels = 560.0F;

}  // namespace

const char* DemoHelpHud::Footer3D() noexcept {
    return "H help | TAB menu | ESC back | F3 FPS";
}

const char* DemoHelpHud::Footer2D() noexcept {
    return "H help | TAB menu | ESC back | F3 FPS";
}

void DemoHelpHud::ToggleGlobalVisible() noexcept {
    g_demoHelpHudVisible = !g_demoHelpHudVisible;
}

bool DemoHelpHud::IsGlobalVisible() noexcept {
    return g_demoHelpHudVisible;
}

bool DemoHelpHud::ProcessGlobalToggle(IInput& input) noexcept {
    if (!input.IsKeyPressedThisFrame(GLFW_KEY_H)) {
        return false;
    }
    ToggleGlobalVisible();
    if (g_activeHelpHud != nullptr && g_demoHelpHudVisible) {
        g_activeHelpHud->textDirty = true;
    }
    return true;
}

void DemoHelpHud::Mount(
        GameWorld& world,
        const char* demoTitle,
        const Style hudStyle) noexcept {
    Unmount(world);
    mounted = true;
    g_activeHelpHud = this;
    style = hudStyle;
    title = demoTitle != nullptr ? Utf8String(demoTitle) : Utf8String{};
    controlHints.Clear();
    detail.Clear();
    renderedText.Clear();
    lastRendered.Clear();
    textDirty = true;
    layoutFont = world.GetUiFont().Get();
    screenX = DemoHud::kScreenMargin;
    screenY = DemoHud::kScreenMargin;
}

void DemoHelpHud::Unmount(GameWorld& /*world*/) noexcept {
    if (g_activeHelpHud == this) {
        g_activeHelpHud = nullptr;
    }
    mounted = false;
    layoutFont = nullptr;
    renderedText.Clear();
    lastRendered.Clear();
    textDirty = true;
}

void DemoHelpHud::SetControlHints(const char* hints) noexcept {
    const Utf8String next(hints != nullptr ? hints : "");
    if (next == controlHints) {
        return;
    }
    controlHints = next;
    textDirty = true;
}

void DemoHelpHud::SetDetail(const char* detailText) noexcept {
    const Utf8String next(detailText != nullptr ? detailText : "");
    if (next == detail) {
        return;
    }
    detail = next;
    textDirty = true;
}

void DemoHelpHud::SetScreenOffset(const float x, const float y) noexcept {
    screenX = x;
    screenY = y;
}

void DemoHelpHud::AppendWrappedSection(
        const Font* font,
        const Utf8String& section,
        const float fontSizePixels,
        const float maxWidthPixels,
        Array<Utf8String>& outLines) const noexcept {
    if (section.IsEmpty()) {
        return;
    }
    if (font == nullptr || !font->IsValid() || maxWidthPixels <= 2.0F) {
        outLines.PushBack(section);
        return;
    }
    SceneRenderParams params{};
    Ui::UiPaintContext paint(params);
    paint.SetLayoutFont(font);
    Array<Utf8String> wrapped;
    paint.BuildWrappedLines(section, fontSizePixels, maxWidthPixels, wrapped);
    for (std::size_t i = 0; i < wrapped.GetSize(); ++i) {
        outLines.PushBack(wrapped[i]);
    }
}

void DemoHelpHud::RebuildText(const int framebufferWidth) noexcept {
    if (!mounted) {
        return;
    }

    const float wrapWidth = std::clamp(
            static_cast<float>(framebufferWidth) * kWrapWidthFraction,
            kMinWrapWidthPixels,
            kMaxWrapWidthPixels);
    const float fontSize = DemoHud::kFontSizePixels;

    Array<Utf8String> lines;
    auto appendSection = [&](const Utf8String& section) {
        if (section.IsEmpty()) {
            return;
        }
        if (!lines.IsEmpty()) {
            lines.PushBack(Utf8String{});
        }
        AppendWrappedSection(layoutFont, section, fontSize, wrapWidth, lines);
    };

    appendSection(title);
    appendSection(detail);
    appendSection(controlHints);
    appendSection(Utf8String(style == Style::LightOnDark ? Footer3D() : Footer2D()));

    Utf8String next;
    for (std::size_t i = 0; i < lines.GetSize(); ++i) {
        if (i > 0) {
            next.AppendUtf8("\n");
        }
        next.AppendUtf8(lines[i].CStr());
    }

    if (next == lastRendered) {
        textDirty = false;
        return;
    }
    lastRendered = next;
    renderedText = next;
    textDirty = false;
}

void DemoHelpHud::Update(const FrameTiming& timing, const int framebufferWidth) noexcept {
    (void)timing;
    if (!mounted || !g_demoHelpHudVisible) {
        return;
    }
    if (!textDirty) {
        return;
    }
    RebuildText(framebufferWidth > 0 ? framebufferWidth : 1280);
}

void DemoHelpHud::Update(const FrameTiming& timing, IEngineContext& context) noexcept {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    Update(timing, fbW > 0 ? fbW : 1280);
}

void DemoHelpHud::PatchSceneRenderParams(SceneRenderParams& params, GameWorld& world) const noexcept {
    if (!mounted || !g_demoHelpHudVisible || renderedText.IsEmpty()) {
        return;
    }
    if (!params.uiFont) {
        params.uiFont = world.GetUiFont();
    }
    if (!params.uiBoldFont) {
        params.uiBoldFont = world.GetUiBoldFont();
    }

    ScreenTextDraw draw{};
    draw.text = renderedText;
    draw.x = screenX;
    draw.y = screenY;
    draw.sizePixels = DemoHud::kFontSizePixels;
    draw.color = style == Style::LightOnDark ? DemoHud::kLightTextColor : DemoHud::kDarkTextColor;
    draw.alpha = DemoHud::kTextAlpha;
    draw.paintOrder = params.NextUiPaintOrder();
    params.screenTexts.PushBack(MoveTemp(draw));
}

}  // namespace Spark
