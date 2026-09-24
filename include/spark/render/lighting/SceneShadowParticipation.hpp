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
        p.castOverride_ = true;
        p.receiveOverride_ = true;
        return p;
    }

    [[nodiscard]] static SceneShadowParticipation None() noexcept {
        SceneShadowParticipation p;
        p.castOverride_ = false;
        p.receiveOverride_ = false;
        return p;
    }

    [[nodiscard]] static SceneShadowParticipation ReceiveOnly() noexcept {
        SceneShadowParticipation p;
        p.castOverride_ = false;
        p.receiveOverride_ = true;
        return p;
    }

    [[nodiscard]] static SceneShadowParticipation CastOnly() noexcept {
        SceneShadowParticipation p;
        p.castOverride_ = true;
        p.receiveOverride_ = false;
        return p;
    }

    SceneShadowParticipation& WithCastOverride(const bool casts) noexcept {
        castOverride_ = casts;
        return *this;
    }

    SceneShadowParticipation& WithReceiveOverride(const bool receives) noexcept {
        receiveOverride_ = receives;
        return *this;
    }

    SceneShadowParticipation& ClearCastOverride() noexcept {
        castOverride_.Reset();
        return *this;
    }

    SceneShadowParticipation& ClearReceiveOverride() noexcept {
        receiveOverride_.Reset();
        return *this;
    }

    SceneShadowParticipation& ClearAllOverrides() noexcept {
        castOverride_.Reset();
        receiveOverride_.Reset();
        return *this;
    }

    [[nodiscard]] bool HasCastOverride() const noexcept { return castOverride_.HasValue(); }
    [[nodiscard]] bool HasReceiveOverride() const noexcept { return receiveOverride_.HasValue(); }
    [[nodiscard]] Optional<bool> GetCastOverride() const noexcept { return castOverride_; }
    [[nodiscard]] Optional<bool> GetReceiveOverride() const noexcept { return receiveOverride_; }

    /** Applies explicit overrides onto default shadow flag bits. */
    [[nodiscard]] std::int32_t ResolveAgainstDefault(const std::int32_t defaultFlags) const noexcept;

    void ApplyTo(MaterialComponent& material) const noexcept;
    [[nodiscard]] static SceneShadowParticipation FromMaterial(const MaterialComponent& material) noexcept;

private:
    Optional<bool> castOverride_{};
    Optional<bool> receiveOverride_{};
};

}  // namespace Spark
