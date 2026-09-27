#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/scene/tilemap/TilemapDocument.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class TilemapComponent;

enum class TilemapEditIssueKind : std::uint8_t {
    EmptyMap = 0,
    MissingAtlasFile = 1,
    LayerCellCountMismatch = 2,
    OrphanTileId = 3,
    ObjectMarkerOffMap = 4,
};

class TilemapEditIssue final {
public:
    TilemapEditIssueKind kind = TilemapEditIssueKind::EmptyMap;
    Utf8String message{};

    [[nodiscard]] TilemapEditIssueKind GetKind() const noexcept { return kind; }
    [[nodiscard]] const Utf8String& GetMessage() const noexcept { return message; }
};

class TilemapEditValidationReport final {
public:
    [[nodiscard]] bool IsClean() const noexcept { return issues.IsEmpty(); }
    [[nodiscard]] const Array<TilemapEditIssue>& GetIssues() const noexcept { return issues; }
    [[nodiscard]] std::size_t GetIssueCount() const noexcept { return issues.GetSize(); }

    /** One line per issue, newline-separated. */
    [[nodiscard]] Utf8String FormatSummary() const;

    Array<TilemapEditIssue> issues{};
};

/** Checks <c>TilemapDocument</c> consistency (tile ids, layers, atlas paths, object markers). */
class TilemapEditValidator final {
public:
    class Options {
    public:
        bool checkAtlasFiles = true;
        bool checkTileIds = true;
        bool checkLayerCellCounts = true;
        bool checkObjectMarkers = true;
        /** When set, overrides atlas cell count inferred from document tilesets. */
        std::uint32_t runtimeMaxTileIdExclusive = 0U;
    };

    [[nodiscard]] TilemapEditValidationReport Validate(const TilemapDocument& document) const noexcept;
    [[nodiscard]] TilemapEditValidationReport Validate(
            const TilemapDocument& document,
            const Options& options) const noexcept;

    [[nodiscard]] TilemapEditValidationReport ValidateFromOwner(const GameObject& owner) const noexcept;
};

}  // namespace Spark
