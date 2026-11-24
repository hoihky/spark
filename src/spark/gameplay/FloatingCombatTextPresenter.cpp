#include "spark/gameplay/FloatingCombatTextPresenter.hpp"

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/camera/WorldSpaceScreenProject.hpp"

#include <cstdio>
#include <cmath>

namespace Spark {

void FloatingCombatTextPresenter::Clear() noexcept {
    entries.Clear();
}

void FloatingCombatTextPresenter::PushEntry(Entry entry) {
    if (entries.GetSize() >= kMaxActive) {
        entries.RemoveAt(0);
    }
    entries.PushBack(MoveTemp(entry));
}

void FloatingCombatTextPresenter::SpawnAtWorld(
        const Vector3& worldAnchor,
        Utf8String text,
        const FloatingCombatTextStyle* styleOverride) {
    const FloatingCombatTextStyle& style = styleOverride != nullptr ? *styleOverride : defaultStyle;
    Entry entry{};
    entry.worldAnchor = worldAnchor;
    entry.text = MoveTemp(text);
    entry.lifetimeSeconds = style.lifetimeSeconds;
    entry.riseWorldUnitsPerSecond = style.riseWorldUnitsPerSecond;
    entry.fontSizePixels = style.fontSizePixels;
    entry.color = style.color;
    entry.alphaStart = style.alphaStart;
    entry.useOverlayLayer = style.useOverlayLayer;
    PushEntry(MoveTemp(entry));
}

void FloatingCombatTextPresenter::SpawnDamage(
        const Vector3& worldAnchor,
        const float appliedAmount,
        const FloatingCombatTextStyle* styleOverride) {
    if (appliedAmount <= 0.0F) {
        return;
    }
    char buf[32]{};
    std::snprintf(buf, sizeof(buf), "%.0f", appliedAmount);
    SpawnAtWorld(worldAnchor, Utf8String(buf), styleOverride);
}

void FloatingCombatTextPresenter::Advance(const float deltaTimeSeconds) noexcept {
    if (deltaTimeSeconds <= 0.0F || entries.IsEmpty()) {
        return;
    }
    for (std::size_t i = 0; i < entries.GetSize();) {
        Entry& entry = entries[i];
        entry.ageSeconds += deltaTimeSeconds;
        entry.worldAnchor.y += entry.riseWorldUnitsPerSecond * deltaTimeSeconds;
        if (entry.ageSeconds >= entry.lifetimeSeconds) {
            entries.RemoveAt(i);
            continue;
        }
        ++i;
    }
}

void FloatingCombatTextPresenter::AppendDraws(
        SceneRenderParams& params,
        const float framebufferWidth,
        const float framebufferHeight) const noexcept {
    if (entries.IsEmpty() || framebufferWidth <= 0.0F || framebufferHeight <= 0.0F) {
        return;
    }
    for (std::size_t i = 0; i < entries.GetSize(); ++i) {
        const Entry& entry = entries[i];
        float sx = 0.0F;
        float sy = 0.0F;
        if (!TryWorldToScreenPixels(params.viewProjection, entry.worldAnchor, framebufferWidth, framebufferHeight, sx, sy)) {
            continue;
        }
        const float lifeT = entry.lifetimeSeconds > 0.0F ? (entry.ageSeconds / entry.lifetimeSeconds) : 1.0F;
        const float alpha = entry.alphaStart * std::max(0.0F, 1.0F - lifeT);

        ScreenTextDraw draw{};
        draw.text = entry.text;
        draw.x = sx;
        draw.y = sy;
        draw.sizePixels = entry.fontSizePixels;
        draw.color = entry.color;
        draw.alpha = alpha;
        draw.bold = true;
        draw.paintOrder = params.NextUiPaintOrder();

        if (entry.useOverlayLayer) {
            params.screenOverlayTexts.PushBack(MoveTemp(draw));
        } else {
            params.screenTexts.PushBack(MoveTemp(draw));
        }
    }
}

}  // namespace Spark