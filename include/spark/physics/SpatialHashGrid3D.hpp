#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/Collision3D.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class GameWorld;

/**
 * Uniform-cell 3D spatial hash for static collider AABBs (broad-phase for sphere dynamics / character motors).
 */
class SpatialHashGrid3D {
public:
    void Clear() noexcept;
    void SetCellSize(float worldCellSize) noexcept;

    void InsertIndexedAabb(std::uint32_t payloadIndex, const CollisionAabb3& worldAabb);

    void QueryUniquePayloadIndices(const CollisionAabb3& queryRegion, Array<std::uint32_t>& outUniqueIndices) const;

    [[nodiscard]] float GetCellSize() const noexcept { return cellSize; }

private:
    struct CellKey {
        int ix = 0;
        int iy = 0;
        int iz = 0;
        [[nodiscard]] bool operator==(const CellKey& o) const noexcept {
            return ix == o.ix && iy == o.iy && iz == o.iz;
        }
    };

    struct CellKeyHash {
        [[nodiscard]] std::size_t operator()(const CellKey& k) const noexcept {
            return static_cast<std::size_t>(k.ix * 73856093) ^ static_cast<std::size_t>(k.iy * 19349663) ^
                    static_cast<std::size_t>(k.iz * 83492791);
        }
    };

    struct CellKeyEq {
        [[nodiscard]] bool operator()(const CellKey& a, const CellKey& b) const noexcept { return a == b; }
    };

    float cellSize = 2.0F;
    float invCellSize = 0.5F;
    HashMap<CellKey, Array<std::uint32_t>, CellKeyHash, CellKeyEq> buckets{};
    /** Scratch for deduplicating query results; not part of the stored grid topology. */
    mutable HashMap<std::uint32_t, std::uint8_t, DefaultHash<std::uint32_t>, DefaultKeyEqual<std::uint32_t>>
            queryDedupe{};
};

}  // namespace Spark
