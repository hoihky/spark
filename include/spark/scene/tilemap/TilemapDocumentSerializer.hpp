#pragma once

#include "spark/scene/tilemap/TilemapDocument.hpp"

namespace Spark {

/** Reads/writes the versioned <c>spark_tilemap_v1</c> sidecar format. */
class TilemapDocumentSerializer {
public:
    [[nodiscard]] bool ReadFromFile(const char* path, TilemapDocument& out) const;
    [[nodiscard]] bool ReadFromString(const char* text, TilemapDocument& out) const;
    [[nodiscard]] bool WriteToFile(const TilemapDocument& document, const char* path) const;
    [[nodiscard]] bool WriteToString(const TilemapDocument& document, Utf8String& out) const;
};

}  // namespace Spark
