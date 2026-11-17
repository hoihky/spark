#pragma once

#include "spark/core/Optional.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"

#include <cstdint>

namespace Spark {

class MaterialComponent;

/**
 * Per-material shadow cast/receive intent for directional CSM.
 *
 * Unset fields defer to <c>SceneRenderParams::shadowsCastByDefault</c> /
 * <c>shadowsReceiveByDefault</c> and scene-submit heuristics (terrain, submerged props).
 *
 * Use <c>ApplyTo</c> on <c>MaterialComponent</c> or <c>ResolveAgainstDefault</c> when building draw items.
 */
class SceneShadowParticipation {
public:
    /** No overrides — scene defaults and built-in heuristics apply. */
    [[nodiscard]] static SceneShadowParticipation Default() noexcept { return {}; }

    [[nodiscard]] static SceneShadowParticipation CastAndReceive() noexcept {
        SceneShadowParticipation p;
        p.castOverride = true;
        p.receiveOverride = true;
        return p;
    }

    [[nodiscard]] static SceneShadowParticipation None() noexcept {
        SceneShadowParticipation p;
        p.castOverride = false;
        p.receiveOverride = false;
        return p;
    }

    [[nodiscard]] static SceneShadowParticipation ReceiveOnly() noexcept {
        SceneShadowParticipation p;
        p.castOverride = false;
        p.receiveOverride = true;
        return p;
    }

    [[nodiscard]] static SceneShadowParticipation CastOnly() noexcept {
        SceneShadowParticipation p;
        p.castOverride = true;
        p.receiveOverride = false;
        return p;
    }

    SceneShadowParticipation& WithCastOverride(const bool casts) noexcept {
        castOverride = casts;
        return *this;
    }

    SceneShadowParticipation& WithReceiveOverride(const bool receives) noexcept {
        receiveOverride = receives;
        return *this;
    }

    SceneShadowParticipation& ClearCastOverride() noexcept {
        castOverride.Reset();
        return *this;
    }

    SceneShadowParticipation& ClearReceiveOverride() noexcept {
        receiveOverride.Reset();
        return *this;
    }

    SceneShadowParticipation& ClearAllOverrides() noexcept {
        castOverride.Reset();
        receiveOverride.Reset();
        return *this;
    }

    [[nodiscard]] bool HasCastOverride() const noexcept { return castOverride.HasValue(); }
    [[nodiscard]] bool HasReceiveOverride() const noexcept { return receiveOverride.HasValue(); }
    [[nodiscard]] Optional<bool> GetCastOverride() const noexcept { return castOverride; }
    [[nodiscard]] Optional<bool> GetReceiveOverride() const noexcept { return receiveOverride; }

    /** Applies explicit overrides onto default shadow flag bits. */
    [[nodiscard]] std::int32_t ResolveAgainstDefault(const std::int32_t defaultFlags) const noexcept;

    void ApplyTo(MaterialComponent& material) const noexcept;
    [[nodiscard]] static SceneShadowParticipation FromMaterial(const MaterialComponent& material) noexcept;

private:
    Optional<bool> castOverride{};
    Optional<bool> receiveOverride{};
};

}  // namespace Spark
