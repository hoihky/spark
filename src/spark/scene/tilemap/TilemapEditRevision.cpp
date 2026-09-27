#include "spark/scene/tilemap/TilemapEditRevision.hpp"

#include <algorithm>

namespace Spark {

TilemapCellRegion TilemapCellRegion::SingleCell(
        const std::uint32_t layer,
        const std::uint32_t cellX,
        const std::uint32_t cellY) noexcept {
    TilemapCellRegion region{};
    region.layerIndex = layer;
    region.minX = cellX;
    region.maxX = cellX;
    region.minY = cellY;
    region.maxY = cellY;
    return region;
}

TilemapCellRegion TilemapCellRegion::WholeMap(
        const std::uint32_t layer,
        const std::uint32_t mapWidth,
        const std::uint32_t mapHeight) noexcept {
    TilemapCellRegion region{};
    region.layerIndex = layer;
    if (mapWidth == 0U || mapHeight == 0U) {
        region.maxX = 0U;
        region.maxY = 0U;
        return region;
    }
    region.minX = 0U;
    region.minY = 0U;
    region.maxX = mapWidth - 1U;
    region.maxY = mapHeight - 1U;
    return region;
}

void TilemapCellRegion::ExpandToInclude(const std::uint32_t cellX, const std::uint32_t cellY) noexcept {
    if (IsEmpty()) {
        minX = cellX;
        maxX = cellX;
        minY = cellY;
        maxY = cellY;
        return;
    }
    minX = std::min(minX, cellX);
    maxX = std::max(maxX, cellX);
    minY = std::min(minY, cellY);
    maxY = std::max(maxY, cellY);
}

void TilemapCellRegion::ExpandMargin(
        const std::uint32_t margin,
        const std::uint32_t mapWidth,
        const std::uint32_t mapHeight) noexcept {
    if (IsEmpty() || mapWidth == 0U || mapHeight == 0U) {
        return;
    }
    if (margin == 0U) {
        return;
    }
    minX = (minX > margin) ? minX - margin : 0U;
    minY = (minY > margin) ? minY - margin : 0U;
    maxX = std::min(maxX + margin, mapWidth - 1U);
    maxY = std::min(maxY + margin, mapHeight - 1U);
}

TilemapCellRegion TilemapCellRegion::FromRevision(
        const TilemapEditRevision& revision,
        const std::uint32_t mapWidth,
        const std::uint32_t mapHeight,
        const std::uint32_t margin) noexcept {
    if (mapWidth == 0U || mapHeight == 0U) {
        return {};
    }
    if (revision.kind == TilemapEditChangeKind::FullMap || revision.kind == TilemapEditChangeKind::MapResize ||
        revision.dirtyRegion.IsEmpty()) {
        return WholeMap(kInvalidLayer, mapWidth, mapHeight);
    }
    TilemapCellRegion region = revision.dirtyRegion;
    region.ExpandMargin(margin, mapWidth, mapHeight);
    return region;
}

void TilemapCellRegion::UnionWith(const TilemapCellRegion& other) noexcept {
    if (other.IsEmpty()) {
        return;
    }
    if (IsEmpty()) {
        *this = other;
        return;
    }
    minX = std::min(minX, other.minX);
    maxX = std::max(maxX, other.maxX);
    minY = std::min(minY, other.minY);
    maxY = std::max(maxY, other.maxY);
    if (layerIndex == kInvalidLayer) {
        layerIndex = other.layerIndex;
    }
}

TilemapEditRevision TilemapRevisionTracker::Bump(
        const TilemapEditChangeKind kind,
        const TilemapCellRegion& region) noexcept {
    ++currentRevision;
    lastRevision.revision = currentRevision;
    lastRevision.kind = kind;
    lastRevision.dirtyRegion = region;
    return lastRevision;
}

TilemapEditRevision TilemapRevisionTracker::RecordCellChange(
        const std::uint32_t layerIndex,
        const std::uint32_t cellX,
        const std::uint32_t cellY) noexcept {
    TilemapCellRegion region = TilemapCellRegion::SingleCell(layerIndex, cellX, cellY);
    if (!lastRevision.dirtyRegion.IsEmpty() && lastRevision.revision > 0U &&
        lastRevision.dirtyRegion.layerIndex == layerIndex &&
        lastRevision.kind == TilemapEditChangeKind::CellPaint) {
        region = lastRevision.dirtyRegion;
        region.ExpandToInclude(cellX, cellY);
    }
    return Bump(TilemapEditChangeKind::CellPaint, region);
}

TilemapEditRevision TilemapRevisionTracker::RecordRegionChange(
        const std::uint32_t layerIndex,
        const TilemapCellRegion& region,
        const TilemapEditChangeKind kind) noexcept {
    TilemapCellRegion merged = region;
    merged.layerIndex = layerIndex;
    if (!lastRevision.dirtyRegion.IsEmpty() && lastRevision.revision > 0U &&
        lastRevision.dirtyRegion.layerIndex == layerIndex && lastRevision.kind == kind) {
        merged.UnionWith(lastRevision.dirtyRegion);
    }
    return Bump(kind, merged);
}

TilemapEditRevision TilemapRevisionTracker::RecordFullMapChange(const TilemapEditChangeKind kind) noexcept {
    TilemapCellRegion region{};
    region.layerIndex = TilemapCellRegion::kInvalidLayer;
    region.minX = 0U;
    region.minY = 0U;
    region.maxX = 0U;
    region.maxY = 0U;
    return Bump(kind, region);
}

void TilemapRevisionTracker::Reset() noexcept {
    currentRevision = 0;
    lastRevision = {};
}

}  // namespace Spark
