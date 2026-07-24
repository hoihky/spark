#pragma once

#include "spark/scene/tilemap/TilemapDocument.hpp"

namespace Spark {

struct TmxImportResult {
    bool success = false;
    Utf8String errorMessage{};
};

/**
 * Imports orthogonal Tiled <c>.tmx</c> maps (CSV tile layers, external <c>.tsx</c> tilesets,
 * object groups). Embedded tilesets and base64 layer encoding are supported.
 */
class TmxImporter {
public:
    /** @param tmxPath Path to the <c>.tmx</c> file. Relative paths resolve from its directory. */
    [[nodiscard]] TmxImportResult ImportFromFile(const char* tmxPath, TilemapDocument& outDocument) const;
};

/** Decodes a Tiled global tile id into atlas id and flip flags (orthogonal). */
void DecodeTiledGid(
        std::uint32_t gid,
        std::uint32_t& outTileId,
        bool& outFlipH,
        bool& outFlipV,
        bool& outFlipDiagonal) noexcept;

}  // namespace Spark
