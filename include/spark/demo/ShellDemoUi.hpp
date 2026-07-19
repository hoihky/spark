#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"

#include "spark/core/Array.hpp"
#include "spark/gui/GuiPaintContext.hpp"
#include "spark/gui/GuiTypes.hpp"
#include "spark/gui/Widget.hpp"

#include <functional>

namespace Spark {

/**
 * Launcher theme bar: caption + prev/next buttons cycling presets (no dropdown popup).
 */
class LauncherThemeRow final : public Spark::Gui::Widget {
public:
    LauncherThemeRow();
    void Arrange(const Spark::Gui::Rect& r) override;
    void Paint(Spark::Gui::GuiPaintContext& ctx) const override;

    void SetOnThemeCycle(std::function<void(int delta)> fn) { onThemeCycle = Spark::MoveTemp(fn); }
    void RefreshThemeName();

    /** Row height for the parent layout (design pixels at uiScale 1). */
    [[nodiscard]] static float DesignHeight() noexcept { return 48.0F; }

private:
    std::function<void(int)> onThemeCycle{};
};

/** Launcher: single child (demo list) or theme row + list centered with padding. */
class LauncherMenuLayout final : public Spark::Gui::Widget {
public:
    explicit LauncherMenuLayout(float minListHeight) noexcept;
    void Arrange(const Spark::Gui::Rect& r) override;
    void Paint(Spark::Gui::GuiPaintContext& ctx) const override;
    [[nodiscard]] Spark::Gui::Widget* FindDeepestHover(float x, float y) override;

private:
    float minListH = 160.0F;
};

/** Stacks children vertically with a fixed row height (simple form layout). */
class GuiStackPanel final : public Spark::Gui::Widget {
public:
    GuiStackPanel();
    explicit GuiStackPanel(float rowH, float gap = 8.0F);
    void Arrange(const Spark::Gui::Rect& r) override;
    void Paint(Spark::Gui::GuiPaintContext& ctx) const override;

private:
    float rowHeight = 32.0F;
    float spacing = 8.0F;
};

void MountUiFont(GameWorld& w);

}  // namespace Spark
