#pragma once

#include "spark/scene/tilemap/TilemapDocument.hpp"

#include <cstdint>

namespace Spark {

/**
 * Imports orthogonal Tiled <c>.tmx</c> maps (CSV tile layers, external <c>.tsx</c> tilesets,
 * object groups). Embedded tilesets and base64 layer encoding are supported.
 */
class TmxImporter final {
public:
    /** Decodes a Tiled global tile id (flip flags + local id bits). */
    class Gid final {
    public:
        explicit Gid(std::uint32_t rawGid) noexcept;

        [[nodiscard]] std::uint32_t GetTileId() const noexcept { return tileId; }
        [[nodiscard]] bool FlipH() const noexcept { return flipH; }
        [[nodiscard]] bool FlipV() const noexcept { return flipV; }
        [[nodiscard]] bool FlipDiagonal() const noexcept { return flipDiagonal; }

    private:
        std::uint32_t tileId = 0U;
        bool flipH = false;
        bool flipV = false;
        bool flipDiagonal = false;
    };

    class Result {
    public:
        [[nodiscard]] bool IsSuccess() const noexcept { return success; }
        [[nodiscard]] const Utf8String& GetErrorMessage() const noexcept { return errorMessage; }

        bool success = false;
        Utf8String errorMessage{};
    };

    /** @param tmxPath Path to the <c>.tmx</c> file. Relative paths resolve from its directory. */
    [[nodiscard]] Result ImportFromFile(const char* tmxPath, TilemapDocument& outDocument) const;
};

}  // namespace Spark
