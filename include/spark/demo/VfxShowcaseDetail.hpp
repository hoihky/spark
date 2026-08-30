#pragma once

#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/scene/vfx/VfxLibrary.hpp"

#include <cstdint>
#include <cstring>

namespace Spark {

namespace VfxShowcaseDetail {

enum class Category : std::uint8_t {
    Weather = 0,
    Ambient,
    Combat,
    Rpg,
    Composite,
    Count
};

struct Entry {
    const char* label;
    const char* assetKey;
    Category category;
    bool isBurst;
    bool isComposite;
    std::uint32_t defaultBurstCount;
};

constexpr int kEntryCount = 35;

inline constexpr Entry kCatalog[kEntryCount] = {
        {"[Weather] Snow", "snow", Category::Weather, false, false, 0},
        {"[Weather] Rain", "rain", Category::Weather, false, false, 0},
        {"[Weather] Leaves", "leaves", Category::Weather, false, false, 0},
        {"[Ambient] Fire", "fire", Category::Ambient, false, false, 0},
        {"[Ambient] Ground fire", "ground_fire", Category::Ambient, false, false, 0},
        {"[Ambient] Embers", "embers", Category::Ambient, false, false, 0},
        {"[Ambient] Smoke", "smoke", Category::Ambient, false, false, 0},
        {"[Ambient] Sparkle", "sparkle", Category::Ambient, false, false, 0},
        {"[Ambient] Electric sparks", "electric", Category::Ambient, false, false, 0},
        {"[Ambient] Holy light", "holy_light", Category::Ambient, false, false, 0},
        {"[Ambient] Rocket trail", "rocket_trail", Category::Ambient, false, false, 0},
        {"[Ambient] Meteor trail", "meteor_trail", Category::Ambient, false, false, 0},
        {"[Combat] Explosion", "explosion", Category::Combat, true, false, 112},
        {"[Combat] Shockwave", "shockwave", Category::Combat, true, false, 72},
        {"[Combat] Impact", "impact", Category::Combat, true, false, 56},
        {"[Combat] Laser hit", "laser_hit", Category::Combat, true, false, 36},
        {"[Combat] Ice shatter", "ice_shatter", Category::Combat, true, false, 64},
        {"[Combat] Magic bolt", "magic_bolt", Category::Combat, false, false, 0},
        {"[Combat] Muzzle flash", "muzzle_flash", Category::Combat, true, false, 28},
        {"[Combat] Blood splatter", "blood", Category::Combat, true, false, 42},
        {"[Combat] Dust puff", "dust", Category::Combat, true, false, 24},
        {"[Combat] Water splash", "water_splash", Category::Combat, true, false, 48},
        {"[RPG] Heal aura", "heal", Category::Rpg, false, false, 0},
        {"[RPG] Level up", "level_up", Category::Rpg, true, false, 88},
        {"[RPG] Poison cloud", "poison", Category::Rpg, false, false, 0},
        {"[RPG] Curse mist", "curse", Category::Rpg, false, false, 0},
        {"[RPG] Shield aura", "aura", Category::Rpg, false, false, 0},
        {"[RPG] Soul fade", "soul", Category::Rpg, false, false, 0},
        {"[RPG] Loot sparkle", "loot_sparkle", Category::Rpg, false, false, 0},
        {"[RPG] Dash trail", "dash_trail", Category::Rpg, false, false, 0},
        {"[Composite] Fireworks", "fireworks", Category::Composite, true, true, 0},
        {"[Composite] Confetti", "confetti", Category::Composite, true, true, 0},
        {"[Composite] Meteor strike", "meteor_strike", Category::Composite, true, true, 0},
        {"[Composite] Magic impact", "magic_impact", Category::Composite, true, true, 0},
        {"[Composite] Smoke grenade", "smoke_grenade", Category::Composite, true, true, 0},
};

inline const char* CategoryName(const Category category) noexcept {
    switch (category) {
    case Category::Weather:
        return "Weather";
    case Category::Ambient:
        return "Ambient";
    case Category::Combat:
        return "Combat";
    case Category::Rpg:
        return "RPG";
    case Category::Composite:
        return "Composite";
    case Category::Count:
        break;
    }
    return "VFX";
}

inline void ApplyEntryToEmitter(const int entryIndex, ParticleEmitterComponent& pe) {
    const int idx = entryIndex >= 0 && entryIndex < kEntryCount ? entryIndex : 0;
    const Entry& entry = kCatalog[idx];
    if (entry.isComposite) {
        pe.SetEmitterEnabled(false);
        pe.ClearParticles();
        return;
    }
    if (VfxLibrary::TryApplyBuiltinByName(entry.assetKey, pe)) {
        pe.ClearParticles();
        return;
    }
    VfxLibrary::ApplyBuiltin(VfxBuiltinId::Fire, pe);
}

inline std::uint32_t BurstCountForEntry(const int entryIndex) noexcept {
    const int idx = entryIndex >= 0 && entryIndex < kEntryCount ? entryIndex : 0;
    const Entry& entry = kCatalog[idx];
    if (entry.defaultBurstCount > 0) {
        return entry.defaultBurstCount;
    }
    for (std::uint8_t i = 0; i < static_cast<std::uint8_t>(VfxBuiltinId::Count); ++i) {
        const auto id = static_cast<VfxBuiltinId>(i);
        if (VfxLibrary::GetBuiltinName(id) != nullptr
            && entry.assetKey != nullptr
            && std::strcmp(VfxLibrary::GetBuiltinName(id), entry.assetKey) == 0) {
            return VfxLibrary::GetDefaultBurstCount(id);
        }
    }
    return 48;
}

}  // namespace VfxShowcaseDetail

}  // namespace Spark
