#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/scene/lod/MeshLodLevel.hpp"

namespace Spark {

/**
 * Distance-based mesh / skinned LOD metadata (OPEN_WORLD C1, FOLIAGE F4, ANIMATION M7-07).
 * Submit and {@link Scene::ForEachDrawableLod} read this when present on the drawable.
 */
class MeshLodComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::MeshLod;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] const Array<MeshLodLevel>& GetLevels() const noexcept { return levels; }
    [[nodiscard]] Array<MeshLodLevel>& GetLevels() noexcept { return levels; }

    void SetHysteresisMeters(float meters) noexcept { hysteresisMeters = meters; }
    [[nodiscard]] float GetHysteresisMeters() const noexcept { return hysteresisMeters; }

    void SetHorizontalDistance(bool enabled) noexcept { horizontalDistance = enabled; }
    [[nodiscard]] bool UsesHorizontalDistance() const noexcept { return horizontalDistance; }

    [[nodiscard]] std::uint32_t GetActiveLodIndex() const noexcept { return activeLodIndex; }

    /** Selects LOD for the current camera distance; updates hysteresis state. */
    [[nodiscard]] std::uint32_t SelectLodIndex(float cameraDistanceMeters) noexcept;

    void Clear() noexcept;

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    void SetEnabled(bool value) noexcept { enabled = value; }

private:
    Array<MeshLodLevel> levels{};
    float hysteresisMeters = 2.0F;
    bool horizontalDistance = false;
    bool enabled = true;
    std::uint32_t activeLodIndex = 0U;
};

}  // namespace Spark