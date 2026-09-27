#pragma once

#include <cstdint>

namespace Spark {

struct TilemapEditRevision;

/** Inclusive cell bounds on the map grid (Spark bottom-left origin). */
struct TilemapCellRegion {
    static constexpr std::uint32_t kInvalidLayer = 0xFFFFFFFFu;

    std::uint32_t minX = 0;
    std::uint32_t minY = 0;
    std::uint32_t maxX = 0;
    std::uint32_t maxY = 0;
    std::uint32_t layerIndex = kInvalidLayer;

    [[nodiscard]] bool IsEmpty() const noexcept { return maxX < minX || maxY < minY; }

    [[nodiscard]] static TilemapCellRegion SingleCell(
            const std::uint32_t layer,
            const std::uint32_t cellX,
            const std::uint32_t cellY) noexcept;

    [[nodiscard]] static TilemapCellRegion WholeMap(
            const std::uint32_t layer,
            const std::uint32_t mapWidth,
            const std::uint32_t mapHeight) noexcept;

    void ExpandToInclude(const std::uint32_t cellX, const std::uint32_t cellY) noexcept;
    void UnionWith(const TilemapCellRegion& other) noexcept;

    /** Grows inclusive bounds by <c>margin</c> cells, clamped to the map. No-op when empty. */
    void ExpandMargin(
            const std::uint32_t margin,
            const std::uint32_t mapWidth,
            const std::uint32_t mapHeight) noexcept;

    /**
     * Dirty rect for derived-data rebakes. Full-map kinds and empty regions use the whole grid;
     * otherwise the revision region is expanded by <c>margin</c> (autotile neighbors).
     */
    [[nodiscard]] static TilemapCellRegion FromRevision(
            const TilemapEditRevision& revision,
            const std::uint32_t mapWidth,
            const std::uint32_t mapHeight,
            const std::uint32_t margin = 1U) noexcept;
};

enum class TilemapEditChangeKind : std::uint8_t {
    CellPaint = 0,
    LayerStructure = 1,
    MapResize = 2,
    ObjectMarkers = 3,
    FullMap = 4,
};

/** Monotonic edit generation for incremental rebakes and editor undo boundaries. */
struct TilemapEditRevision {
    std::uint64_t revision = 0;
    TilemapEditChangeKind kind = TilemapEditChangeKind::CellPaint;
    TilemapCellRegion dirtyRegion{};
};

/**
 * Tracks the latest revision and dirty region. Editors hold one instance per edited map;
 * Phase B edit sessions will bump this after each command.
 */
class TilemapRevisionTracker final {
public:
    [[nodiscard]] std::uint64_t GetCurrentRevision() const noexcept { return currentRevision; }
    [[nodiscard]] const TilemapEditRevision& GetLastRevision() const noexcept { return lastRevision; }

    [[nodiscard]] TilemapEditRevision RecordCellChange(
            const std::uint32_t layerIndex,
            const std::uint32_t cellX,
            const std::uint32_t cellY) noexcept;

    [[nodiscard]] TilemapEditRevision RecordRegionChange(
            const std::uint32_t layerIndex,
            const TilemapCellRegion& region,
            const TilemapEditChangeKind kind = TilemapEditChangeKind::CellPaint) noexcept;

    [[nodiscard]] TilemapEditRevision RecordFullMapChange(
            const TilemapEditChangeKind kind = TilemapEditChangeKind::FullMap) noexcept;

    void Reset() noexcept;

private:
    [[nodiscard]] TilemapEditRevision Bump(
            const TilemapEditChangeKind kind,
            const TilemapCellRegion& region) noexcept;

    std::uint64_t currentRevision = 0;
    TilemapEditRevision lastRevision{};
};

}  // namespace Spark
