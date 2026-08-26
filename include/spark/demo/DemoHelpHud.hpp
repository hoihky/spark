#pragma once

#include "spark/demo/DemoFoundation.hpp"
#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/engine/FrameTiming.hpp"

namespace Spark {

struct SceneRenderParams;
class Font;
class GameWorld;
class IEngineContext;
class IInput;

/**
 * Standard top-left help overlay for shell demos (title, detail, controls, shared footer).
 * Press H to toggle visibility. Text wraps to multiple lines when needed.
 *
 * Help text is patched into <c>SceneRenderParams::screenTexts</c> (not a scene
 * <c>TextOverlayComponent</c>) so toggling visibility cannot trip ECS lifetime issues.
 */
class DemoHelpHud {
public:
    enum class Style : std::uint8_t {
        LightOnDark,
        DarkOnBright,
    };

    void Mount(GameWorld& world, const char* title, Style style = Style::LightOnDark) noexcept;
    void Unmount(GameWorld& world) noexcept;
    [[nodiscard]] bool IsMounted() const noexcept { return mounted; }

    void SetControlHints(const char* hints) noexcept;
    void SetDetail(const char* detail) noexcept;
    void SetScreenOffset(float x, float y) noexcept;

    /** Applies pending text changes (no-op when nothing changed since last draw). */
    void Update(const FrameTiming& timing, int framebufferWidth) noexcept;
    void Update(const FrameTiming& timing, IEngineContext& context) noexcept;

    /** Append help lines to <c>params.screenTexts</c> when globally visible. */
    void PatchSceneRenderParams(SceneRenderParams& params, GameWorld& world) const noexcept;

    /** Global H toggle shared by all demo help overlays. */
    static void ToggleGlobalVisible() noexcept;
    [[nodiscard]] static bool IsGlobalVisible() noexcept;
    static bool ProcessGlobalToggle(IInput& input) noexcept;

    [[nodiscard]] static const char* Footer3D() noexcept;
    [[nodiscard]] static const char* Footer2D() noexcept;

private:
    void RebuildText(int framebufferWidth) noexcept;
    void AppendWrappedSection(
            const Font* font,
            const Utf8String& section,
            float fontSizePixels,
            float maxWidthPixels,
            Array<Utf8String>& outLines) const noexcept;

    Style style = Style::LightOnDark;
    Utf8String title{};
    Utf8String controlHints{};
    Utf8String detail{};
    bool mounted = false;
    bool textDirty = true;
    Utf8String renderedText{};
    Utf8String lastRendered{};
    const Font* layoutFont = nullptr;
    float screenX = DemoHud::kScreenMargin;
    float screenY = DemoHud::kScreenMargin;
};

}  // namespace Spark
