#include "spark/scripting/SparkInterop.h"
#include "spark/scripting/SparkInteropInternal.hpp"

#include "spark/physics/CollisionFilter2D.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>

using namespace Spark::Scripting;

namespace {

[[nodiscard]] Spark::PhysicsQueryFilter2D ToCppFilter(const SparkPhysicsQueryFilter2D& filter) noexcept {
    return {
            .queryCategoryBits = filter.queryCategoryBits,
            .queryMaskBits = filter.queryMaskBits,
            .hitSolids = filter.hitSolids != 0,
            .hitTriggers = filter.hitTriggers != 0,
    };
}

[[nodiscard]] std::uint32_t CopyHits(
        const Spark::Array<Spark::PhysicsQueryHit2D>& src,
        SparkPhysicsQueryHit2D* outHits,
        const std::uint32_t maxHits) noexcept {
    const std::uint32_t total = static_cast<std::uint32_t>(src.GetSize());
    if (outHits == nullptr || maxHits == 0U) {
        return total;
    }
    const std::uint32_t writeCount = (std::min)(total, maxHits);
    for (std::uint32_t i = 0; i < writeCount; ++i) {
        outHits[i].staticColliderIndex = src[i].staticColliderIndex;
        outHits[i].owner = reinterpret_cast<SparkGameObject*>(src[i].owner);
    }
    return total;
}

}  // namespace

extern "C" {

uint16_t spark_collision_filter_2d_layer_bit(const uint32_t layerIndex) {
    return Spark::CollisionFilter2D::LayerBit(layerIndex);
}

uint16_t spark_collision_filter_2d_all_layers_mask(void) {
    return Spark::CollisionFilter2D::AllLayersMask();
}

uint16_t spark_collision_filter_2d_default_category(void) {
    return Spark::CollisionFilter2D::DefaultCategory();
}

uint32_t spark_physics_query_overlap_circle_world_2d(
        SparkGameWorld* world,
        const float centerX,
        const float centerY,
        const float radius,
        const SparkPhysicsQueryFilter2D* filter,
        SparkPhysicsQueryHit2D* outHits,
        const uint32_t maxHits,
        const float cellWorldSize) {
    if (world == nullptr || filter == nullptr) {
        return 0U;
    }
    Spark::Array<Spark::PhysicsQueryHit2D> scratch;
    Spark::QueryOverlapCircleWorld2D(
            *reinterpret_cast<Spark::GameWorld*>(world),
            centerX,
            centerY,
            radius,
            ToCppFilter(*filter),
            scratch,
            cellWorldSize);
    return CopyHits(scratch, outHits, maxHits);
}

uint32_t spark_physics_query_overlap_arc_world_statics_2d(
        SparkGameWorld* world,
        const float originX,
        const float originY,
        const float radius,
        const float dirX,
        const float dirY,
        const float halfAngleRadians,
        const SparkPhysicsQueryFilter2D* filter,
        SparkPhysicsQueryHit2D* outHits,
        const uint32_t maxHits,
        const float cellWorldSize) {
    if (world == nullptr || filter == nullptr) {
        return 0U;
    }
    Spark::Array<Spark::PhysicsQueryHit2D> scratch;
    Spark::QueryOverlapArcWorldStatics2D(
            *reinterpret_cast<Spark::GameWorld*>(world),
            originX,
            originY,
            radius,
            dirX,
            dirY,
            halfAngleRadians,
            ToCppFilter(*filter),
            scratch,
            cellWorldSize);
    return CopyHits(scratch, outHits, maxHits);
}

}  // extern "C"
