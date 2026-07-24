#include "spark/scene/tilemap/TilemapDocumentSerializer.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace Spark {
namespace {

std::uint32_t ComputeTilesV(const TilemapDocumentTileset& set) noexcept {
    if (set.columns == 0U) {
        return 1U;
    }
    const std::uint32_t count = set.tileCount > 0U ? set.tileCount : set.columns;
    return (count + set.columns - 1U) / set.columns;
}

bool WriteLine(std::FILE* f, const char* line) {
    if (f == nullptr || line == nullptr) {
        return false;
    }
    return std::fputs(line, f) >= 0 && std::fputc('\n', f) >= 0;
}

void AppendLine(Utf8String& out, const char* line) {
    out.AppendUtf8(line);
    out.AppendUtf8("\n");
}

void AppendFormat(Utf8String& out, const char* fmt, ...) {
    char buf[512]{};
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    AppendLine(out, buf);
}

bool StartsWith(const char* line, const char* prefix) {
    return line != nullptr && prefix != nullptr && std::strncmp(line, prefix, std::strlen(prefix)) == 0;
}

}  // namespace

bool TilemapDocumentSerializer::WriteToFile(const TilemapDocument& document, const char* path) const {
    Utf8String text{};
    if (!WriteToString(document, text)) {
        return false;
    }
    std::FILE* f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    const std::size_t n = text.ByteLength();
    const std::size_t written = std::fwrite(text.CStr(), 1, n, f);
    std::fclose(f);
    return written == n;
}

bool TilemapDocumentSerializer::WriteToString(const TilemapDocument& document, Utf8String& out) const {
    out.Clear();
    AppendLine(out, TilemapDocument::kMagic);
    AppendFormat(out, "map %u %u %.6f %d", document.mapWidth, document.mapHeight, document.tileWorldSize,
            document.sortOrderBase);
    if (!document.sourceTmxPath.IsEmpty()) {
        AppendFormat(out, "tmx %s", document.sourceTmxPath.CStr());
    }
    for (std::size_t i = 0; i < document.mapProperties.GetSize(); ++i) {
        AppendFormat(out, "prop map %s %s", document.mapProperties[i].key.CStr(),
                document.mapProperties[i].value.CStr());
    }
    for (std::size_t ti = 0; ti < document.tilesets.GetSize(); ++ti) {
        const TilemapDocumentTileset& ts = document.tilesets[ti];
        AppendFormat(out, "tileset %u %s %u %u %u %u %u %u", ts.firstGid, ts.imagePath.CStr(), ts.columns,
                ComputeTilesV(ts), ts.margin, ts.spacing, ts.tileWidth, ts.tileHeight);
    }
    for (std::size_t li = 0; li < document.tileLayers.GetSize(); ++li) {
        const TilemapDocumentTileLayer& layer = document.tileLayers[li];
        AppendFormat(out, "layer %s %d %d %d %d", layer.name.CStr(), layer.visible ? 1 : 0, layer.orderInLayerOffset,
                layer.contributeCollision ? 1 : 0, layer.contributeGameplayGrid ? 1 : 0);
        AppendLine(out, "cells_begin");
        const std::size_t cellCount = layer.cells.GetSize();
        for (std::size_t ci = 0; ci < cellCount; ++ci) {
            const TileCell& cell = layer.cells[ci];
            AppendFormat(out, "%u %u %u %u %u %u %u", static_cast<unsigned>(cell.tileId),
                    static_cast<unsigned>(cell.paintTileId), static_cast<unsigned>(cell.transformFlags),
                    static_cast<unsigned>(cell.tintR), static_cast<unsigned>(cell.tintG),
                    static_cast<unsigned>(cell.tintB), static_cast<unsigned>(cell.tintA));
        }
        AppendLine(out, "cells_end");
    }
    for (std::size_t oi = 0; oi < document.objectLayers.GetSize(); ++oi) {
        const TilemapObjectLayer& ol = document.objectLayers[oi];
        AppendFormat(out, "object_layer %s %d", ol.name.CStr(), ol.visible ? 1 : 0);
        for (std::size_t mi = 0; mi < ol.markers.GetSize(); ++mi) {
            const TilemapObjectMarker& m = ol.markers[mi];
            AppendFormat(out, "object %u %s %s %d %d %.4f %.4f %u", m.id, m.name.CStr(), m.typeId.CStr(), m.cellX,
                    m.cellY, m.offsetX, m.offsetY, static_cast<unsigned>(m.mode));
        }
    }
    return true;
}

bool TilemapDocumentSerializer::ReadFromFile(const char* path, TilemapDocument& out) const {
    if (path == nullptr) {
        return false;
    }
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    const long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz <= 0) {
        std::fclose(f);
        return false;
    }
    Array<char> buf{};
    buf.Resize(static_cast<std::size_t>(sz) + 1U);
    const std::size_t readN = std::fread(buf.GetData(), 1, static_cast<std::size_t>(sz), f);
    std::fclose(f);
    buf[readN] = '\0';
    return ReadFromString(buf.GetData(), out);
}

bool TilemapDocumentSerializer::ReadFromString(const char* text, TilemapDocument& out) const {
    if (text == nullptr) {
        return false;
    }
    out = TilemapDocument{};
    const char* cursor = text;
    const char* lineEnd = std::strchr(cursor, '\n');
    if (lineEnd == nullptr) {
        return false;
    }
    char firstLine[64]{};
    const std::size_t firstLen = static_cast<std::size_t>(lineEnd - cursor);
    if (firstLen >= sizeof(firstLine)) {
        return false;
    }
    std::memcpy(firstLine, cursor, firstLen);
    firstLine[firstLen] = '\0';
    if (std::strcmp(firstLine, TilemapDocument::kMagic) != 0) {
        return false;
    }
    cursor = lineEnd + 1;

    TilemapDocumentTileLayer* currentLayer = nullptr;
    bool inCells = false;

    while (*cursor != '\0') {
        lineEnd = std::strchr(cursor, '\n');
        if (lineEnd == nullptr) {
            lineEnd = cursor + std::strlen(cursor);
        }
        char line[1024]{};
        const std::size_t len = static_cast<std::size_t>(lineEnd - cursor);
        if (len >= sizeof(line)) {
            return false;
        }
        std::memcpy(line, cursor, len);
        line[len] = '\0';
        cursor = (*lineEnd == '\0') ? lineEnd : lineEnd + 1;

        if (line[0] == '\0') {
            continue;
        }
        if (StartsWith(line, "map ")) {
            unsigned w = 0;
            unsigned h = 0;
            float tileWorld = 1.0F;
            int sortBase = 0;
            if (std::sscanf(line, "map %u %u %f %d", &w, &h, &tileWorld, &sortBase) != 4) {
                return false;
            }
            out.mapWidth = w;
            out.mapHeight = h;
            out.tileWorldSize = tileWorld;
            out.sortOrderBase = sortBase;
            continue;
        }
        if (StartsWith(line, "tmx ")) {
            out.sourceTmxPath = Utf8String(line + 4);
            continue;
        }
        if (StartsWith(line, "tileset ")) {
            TilemapDocumentTileset ts{};
            unsigned firstGid = 1;
            unsigned cols = 1;
            unsigned rows = 1;
            unsigned margin = 0;
            unsigned spacing = 0;
            unsigned tw = 16;
            unsigned th = 16;
            char image[512]{};
            if (std::sscanf(line, "tileset %u %511s %u %u %u %u %u %u", &firstGid, image, &cols, &rows, &margin,
                        &spacing, &tw, &th) != 8) {
                return false;
            }
            ts.firstGid = firstGid;
            ts.imagePath = Utf8String(image);
            ts.columns = cols;
            ts.tileCount = cols * rows;
            ts.margin = margin;
            ts.spacing = spacing;
            ts.tileWidth = tw;
            ts.tileHeight = th;
            out.tilesets.PushBack(ts);
            continue;
        }
        if (StartsWith(line, "layer ")) {
            TilemapDocumentTileLayer layer{};
            char name[128]{};
            int visible = 1;
            int order = 0;
            int coll = 1;
            int gameplay = 1;
            if (std::sscanf(line, "layer %127s %d %d %d %d", name, &visible, &order, &coll, &gameplay) != 5) {
                return false;
            }
            layer.name = Utf8String(name);
            layer.visible = visible != 0;
            layer.orderInLayerOffset = order;
            layer.contributeCollision = coll != 0;
            layer.contributeGameplayGrid = gameplay != 0;
            out.tileLayers.PushBack(layer);
            currentLayer = &out.tileLayers.GetLast();
            inCells = false;
            continue;
        }
        if (StartsWith(line, "cells_begin")) {
            inCells = true;
            if (currentLayer != nullptr) {
                currentLayer->cells.Clear();
            }
            continue;
        }
        if (StartsWith(line, "cells_end")) {
            inCells = false;
            continue;
        }
        if (inCells && currentLayer != nullptr) {
            TileCell cell{};
            unsigned tileId = 0;
            unsigned paintId = 0;
            unsigned tf = 0;
            unsigned tr = 255;
            unsigned tg = 255;
            unsigned tb = 255;
            unsigned ta = 255;
            if (std::sscanf(line, "%u %u %u %u %u %u %u", &tileId, &paintId, &tf, &tr, &tg, &tb, &ta) != 7) {
                return false;
            }
            cell.tileId = static_cast<std::uint16_t>(tileId);
            cell.paintTileId = static_cast<std::uint16_t>(paintId);
            cell.transformFlags = static_cast<std::uint8_t>(tf);
            cell.tintR = static_cast<std::uint8_t>(tr);
            cell.tintG = static_cast<std::uint8_t>(tg);
            cell.tintB = static_cast<std::uint8_t>(tb);
            cell.tintA = static_cast<std::uint8_t>(ta);
            currentLayer->cells.PushBack(cell);
            continue;
        }
        if (StartsWith(line, "object_layer ")) {
            TilemapObjectLayer ol{};
            char name[128]{};
            int visible = 1;
            if (std::sscanf(line, "object_layer %127s %d", name, &visible) != 2) {
                return false;
            }
            ol.name = Utf8String(name);
            ol.visible = visible != 0;
            out.objectLayers.PushBack(ol);
            continue;
        }
        if (StartsWith(line, "object ")) {
            if (out.objectLayers.IsEmpty()) {
                return false;
            }
            TilemapObjectMarker marker{};
            char name[128]{};
            char typeId[128]{};
            unsigned mode = 0;
            if (std::sscanf(line, "object %u %127s %127s %d %d %f %f %u", &marker.id, name, typeId, &marker.cellX,
                        &marker.cellY, &marker.offsetX, &marker.offsetY, &mode) != 8) {
                return false;
            }
            marker.name = Utf8String(name);
            marker.typeId = Utf8String(typeId);
            marker.mode = static_cast<TilemapObjectMarkerMode>(mode);
            out.objectLayers.GetLast().markers.PushBack(marker);
            continue;
        }
    }
    return !out.tileLayers.IsEmpty();
}

}  // namespace Spark
