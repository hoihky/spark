#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/Collision2D.hpp"

#include <cstdint>

namespace Spark {

class GameWorld;

/**
 * Uniform-cell spatial hash for 2D broad-phase: O(1) cell buckets, conservative insertion of
 * indexed AABBs, unique index queries. Pairs with parallel <c>Collider2D</c>[] built from ECS
 * (static BoxCollider2D, CircleCollider2D, and/or TilemapCollider2D).
 */
class SpatialHashGrid2D {
public:
    void Clear() noexcept;
    void SetCellSize(float worldCellSize) noexcept;

    /** Inserts payload index into every cell overlapped by [minX,maxX]×[minY,maxY]. */
    void InsertIndexedAabb(std::uint32_t payloadIndex, const CollisionAabb2& worldAabb);

    /** All unique payload indices whose cells overlap the query region (narrow-phase still required). */
    void QueryUniquePayloadIndices(const CollisionAabb2& queryRegion, Array<std::uint32_t>& outUniqueIndices) const;

    [[nodiscard]] float GetCellSize() const noexcept { return cellSize; }

private:
    struct CellKey {
        int ix = 0;
        int iy = 0;
        [[nodiscard]] bool operator==(const CellKey& o) const noexcept { return ix == o.ix && iy == o.iy; }
    };

    struct CellKeyHash {
        [[nodiscard]] std::size_t operator()(const CellKey& k) const noexcept {
            return static_cast<std::size_t>(k.ix * 73856093) ^ static_cast<std::size_t>(k.iy * 19349663);
        }
    };

    struct CellKeyEq {
        [[nodiscard]] bool operator()(const CellKey& a, const CellKey& b) const noexcept { return a == b; }
    };

    float cellSize = 4.0F;
    float invCellSize = 0.25F;
    HashMap<CellKey, Array<std::uint32_t>, CellKeyHash, CellKeyEq> buckets{};
    /** Scratch for deduplicating query results; not part of the stored grid topology. */
    mutable HashMap<std::uint32_t, std::uint8_t, DefaultHash<std::uint32_t>, DefaultKeyEqual<std::uint32_t>>
            queryDedupe{};
};

}  // namespace Spark
