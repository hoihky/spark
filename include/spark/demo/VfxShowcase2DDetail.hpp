#pragma once

#include "spark/scene/vfx/VfxLibrary.hpp"

#include <cstdint>

namespace Spark {
namespace VfxShowcase2DDetail {

struct Entry {
    const char* label;
    const char* assetKey;
    VfxBuiltinId builtin;
};

constexpr int kEntryCount = 14;

inline constexpr Entry kCatalog[kEntryCount] = {
        {"Hit spark", "hit_spark_2d", VfxBuiltinId::HitSpark2D},
        {"Coin pop", "coin_pop_2d", VfxBuiltinId::CoinPop2D},
        {"Jump ring", "jump_ring_2d", VfxBuiltinId::JumpRing2D},
        {"Lantern glow", "lantern_glow_2d", VfxBuiltinId::LanternGlow2D},
        {"Rain splash", "rain_splash_2d", VfxBuiltinId::RainSplash2D},
        {"Slash arc", "slash_arc_2d", VfxBuiltinId::SlashArc2D},
        {"Footstep puff", "footstep_puff_2d", VfxBuiltinId::FootstepPuff2D},
        {"Block impact", "block_impact_2d", VfxBuiltinId::BlockImpact2D},
        {"Magic nova", "magic_nova_2d", VfxBuiltinId::MagicNova2D},
        {"Heal sparkle", "heal_sparkle_2d", VfxBuiltinId::HealSparkle2D},
        {"Poison bubble", "poison_bubble_2d", VfxBuiltinId::PoisonBubble2D},
        {"Shield pulse", "shield_pulse_2d", VfxBuiltinId::ShieldPulse2D},
        {"Water ripple", "water_ripple_2d", VfxBuiltinId::WaterRipple2D},
        {"Ember motif", "ember_motif_2d", VfxBuiltinId::EmberMotif2D},
};

inline const Entry& EntryAt(const int index) noexcept {
    const int clamped = index < 0 ? 0 : (index >= kEntryCount ? kEntryCount - 1 : index);
    return kCatalog[clamped];
}

}  // namespace VfxShowcase2DDetail
}  // namespace Spark
