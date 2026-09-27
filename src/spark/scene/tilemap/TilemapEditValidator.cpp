#include "spark/scene/tilemap/TilemapEditValidator.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/tilemap/TilemapDocumentCapture.hpp"
#include "spark/scene/tilemap/TilemapFileResolve.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <algorithm>

namespace Spark {

namespace {

void PushIssue(TilemapEditValidationReport& report, const TilemapEditIssueKind kind, const char* message) {
    TilemapEditIssue issue{};
    issue.kind = kind;
    issue.message = Utf8String(message != nullptr ? message : "");
    report.issues.PushBack(issue);
}

[[nodiscard]] std::uint32_t InferMaxTileIdExclusive(const TilemapDocument& document) noexcept {
    std::uint32_t maxCells = 0U;
    for (std::size_t i = 0; i < document.tilesets.GetSize(); ++i) {
        const TilemapDocumentTileset& set = document.tilesets[i];
        std::uint32_t count = set.tileCount;
        if (count == 0U && set.columns > 0U) {
            const std::uint32_t rows =
                    set.imageHeight > 0U && set.tileHeight > 0U ? set.imageHeight / set.tileHeight : 1U;
            count = set.columns * rows;
        }
        maxCells = std::max(maxCells, count);
    }
    return maxCells;
}

[[nodiscard]] bool IsOrphanTileId(const std::uint16_t tileId, const std::uint32_t maxExclusive) noexcept {
    return tileId != TileCell::kEmptyTileId && maxExclusive > 0U && tileId >= maxExclusive;
}

void ValidateAtlasFiles(const TilemapDocument& document, TilemapEditValidationReport& report) {
    for (std::size_t i = 0; i < document.tilesets.GetSize(); ++i) {
        const TilemapDocumentTileset& set = document.tilesets[i];
        if (set.imagePath.IsEmpty()) {
            PushIssue(report, TilemapEditIssueKind::MissingAtlasFile, "Tileset has empty image path");
            continue;
        }
        const Utf8String resolved = ResolveTilemapAssetPath(set.imagePath.CStr());
        if (resolved.IsEmpty() || !TilemapFileExists(resolved.CStr())) {
            Utf8String message = Utf8String("Missing tileset image: ");
            message.AppendUtf8(set.imagePath.CStr());
            PushIssue(report, TilemapEditIssueKind::MissingAtlasFile, message.CStr());
        }
    }
}

void ValidateLayerCells(
        const TilemapDocument& document,
        const std::uint32_t maxTileIdExclusive,
        const TilemapEditValidator::Options& options,
        TilemapEditValidationReport& report) {
    if (document.mapWidth == 0U || document.mapHeight == 0U) {
        return;
    }
    const std::size_t expectedCells =
            static_cast<std::size_t>(document.mapWidth) * static_cast<std::size_t>(document.mapHeight);
    for (std::size_t li = 0; li < document.tileLayers.GetSize(); ++li) {
        const TilemapDocumentTileLayer& layer = document.tileLayers[li];
        if (options.checkLayerCellCounts && layer.cells.GetSize() != expectedCells) {
            PushIssue(report, TilemapEditIssueKind::LayerCellCountMismatch, "Tile layer cell count does not match map size");
        }
        if (!options.checkTileIds || maxTileIdExclusive == 0U) {
            continue;
        }
        for (std::size_t ci = 0; ci < layer.cells.GetSize(); ++ci) {
            const TileCell& cell = layer.cells[ci];
            if (IsOrphanTileId(cell.tileId, maxTileIdExclusive)) {
                PushIssue(report, TilemapEditIssueKind::OrphanTileId, "Tile layer references orphan display tile id");
                return;
            }
            if (IsOrphanTileId(cell.paintTileId, maxTileIdExclusive)) {
                PushIssue(report, TilemapEditIssueKind::OrphanTileId, "Tile layer references orphan paint tile id");
                return;
            }
        }
    }
}

void ValidateObjectMarkers(const TilemapDocument& document, TilemapEditValidationReport& report) {
    if (document.mapWidth == 0U || document.mapHeight == 0U) {
        return;
    }
    for (std::size_t oi = 0; oi < document.objectLayers.GetSize(); ++oi) {
        const TilemapObjectLayer& layer = document.objectLayers[oi];
        for (std::size_t mi = 0; mi < layer.markers.GetSize(); ++mi) {
            const TilemapObjectMarker& marker = layer.markers[mi];
            if (marker.cellX < 0 || marker.cellY < 0 ||
                static_cast<std::uint32_t>(marker.cellX) >= document.mapWidth ||
                static_cast<std::uint32_t>(marker.cellY) >= document.mapHeight) {
                PushIssue(report, TilemapEditIssueKind::ObjectMarkerOffMap, "Object marker is outside map bounds");
                return;
            }
        }
    }
}

}  // namespace

Utf8String TilemapEditValidationReport::FormatSummary() const {
    Utf8String out{};
    for (std::size_t i = 0; i < issues.GetSize(); ++i) {
        if (!issues[i].message.IsEmpty()) {
            out.AppendUtf8(issues[i].message.CStr());
        }
        if (i + 1U < issues.GetSize()) {
            out.AppendUtf8("\n");
        }
    }
    return out;
}

TilemapEditValidationReport TilemapEditValidator::Validate(const TilemapDocument& document) const noexcept {
    Options defaultOptions{};
    return Validate(document, defaultOptions);
}

TilemapEditValidationReport TilemapEditValidator::Validate(
        const TilemapDocument& document,
        const Options& options) const noexcept {
    TilemapEditValidationReport report{};
    if (document.mapWidth == 0U || document.mapHeight == 0U || document.tileLayers.IsEmpty()) {
        PushIssue(report, TilemapEditIssueKind::EmptyMap, "Tilemap document has no tile layers or zero size");
        return report;
    }
    if (options.checkAtlasFiles) {
        ValidateAtlasFiles(document, report);
    }
    const std::uint32_t maxTileIdExclusive = options.runtimeMaxTileIdExclusive > 0U
            ? options.runtimeMaxTileIdExclusive
            : InferMaxTileIdExclusive(document);
    ValidateLayerCells(document, maxTileIdExclusive, options, report);
    if (options.checkObjectMarkers) {
        ValidateObjectMarkers(document, report);
    }
    return report;
}

TilemapEditValidationReport TilemapEditValidator::ValidateFromOwner(const GameObject& owner) const noexcept {
    const TilemapDocumentCapturer capturer{};
    const TilemapDocumentCapturer::Result captured = capturer.CaptureFromOwner(owner);
    if (!captured.IsSuccess()) {
        TilemapEditValidationReport report{};
        PushIssue(report, TilemapEditIssueKind::EmptyMap, "Failed to capture tilemap from owner");
        return report;
    }
    Options options{};
    if (const TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>()) {
        if (const SharedPtr<Tileset>& tileset = tilemap->GetTileset(); tileset) {
            options.runtimeMaxTileIdExclusive = tileset->GetCellCount();
        }
    }
    return Validate(captured.GetDocument(), options);
}

}  // namespace Spark
