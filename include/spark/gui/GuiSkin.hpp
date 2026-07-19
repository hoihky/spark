#pragma once

#include "spark/gui/GuiSkinElement.hpp"
#include "spark/gui/GuiSpriteSlice.hpp"

namespace Spark::Gui {

class GuiPaintContext;

/**
 * Strategy interface for UI appearance packs. Controls query slices by semantic element instead of
 * hard-coding atlas coordinates (Open/Closed: open for extension, closed for modification).
 */
class GuiSkin {
public:
    virtual ~GuiSkin() = default;

    /** Returns false when @p element is not defined by this skin (caller falls back to theme colors). */
    [[nodiscard]] virtual bool TryGetSlice(GuiSkinElement element, GuiSpriteSlice& out) const = 0;

    /** Stable identifier for logging / editor (e.g. "sprout_lands"). */
    [[nodiscard]] virtual const char* GetSkinId() const noexcept = 0;

    /** When false, atlas textures failed to load; controls should fall back to theme chrome. */
    [[nodiscard]] virtual bool IsLoaded() const noexcept { return true; }

    /** Registers atlas textures in stable order before widget paint (optional; default no-op). */
    virtual void PrimeUiTextures(GuiPaintContext& ctx) const { (void)ctx; }
};

}  // namespace Spark::Gui
