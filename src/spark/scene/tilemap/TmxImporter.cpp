#include "spark/scene/tilemap/TmxImporter.hpp"

#include "spark/scene/tilemap/TilemapFileResolve.hpp"
#include "spark/scene/tilemap/TileTransform.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Spark {
namespace {

bool ReadFileBytes(const char* path, Array<char>& out) {
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
    if (sz < 0) {
        std::fclose(f);
        return false;
    }
    out.Resize(static_cast<std::size_t>(sz) + 1U);
    const std::size_t readN = std::fread(out.GetData(), 1, static_cast<std::size_t>(sz), f);
    std::fclose(f);
    out[readN] = '\0';
    return true;
}

Utf8String Dirname(const char* path) {
    if (path == nullptr) {
        return Utf8String(".");
    }
    const char* lastSlash = nullptr;
    for (const char* p = path; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            lastSlash = p;
        }
    }
    if (lastSlash == nullptr) {
        return Utf8String(".");
    }
    Utf8String out{};
    for (const char* p = path; p < lastSlash; ++p) {
        const char ch[2] = {*p, '\0'};
        out.AppendUtf8(ch);
    }
    if (out.IsEmpty()) {
        return Utf8String(".");
    }
    return out;
}

Utf8String JoinPath(const Utf8String& dir, const char* relative) {
    if (relative == nullptr || relative[0] == '\0') {
        return dir;
    }
    if (relative[0] == '/') {
        return Utf8String(relative);
    }
    Utf8String out = dir;
    if (!out.IsEmpty()) {
        const char* c = out.CStr();
        const std::size_t n = out.ByteLength();
        if (n > 0 && c[n - 1] != '/' && c[n - 1] != '\\') {
            out.AppendUtf8("/");
        }
    }
    out.AppendUtf8(relative);
    return out;
}

bool TryGetAttribute(const char* tagOpen, const char* tagClose, const char* name, Utf8String& out) {
    if (tagOpen == nullptr || tagClose == nullptr || name == nullptr) {
        return false;
    }
    char pattern[64]{};
    std::snprintf(pattern, sizeof(pattern), "%s=\"", name);
    const char* found = tagOpen;
    while (found < tagClose) {
        found = std::strstr(found, pattern);
        if (found == nullptr || found >= tagClose) {
            char patternSingle[64]{};
            std::snprintf(patternSingle, sizeof(patternSingle), "%s='", name);
            found = std::strstr(tagOpen, patternSingle);
            if (found == nullptr || found >= tagClose) {
                return false;
            }
            found += std::strlen(patternSingle);
            const char* end = std::strchr(found, '\'');
            if (end == nullptr || end > tagClose) {
                return false;
            }
            out.Clear();
            for (const char* p = found; p < end; ++p) {
                const char ch[2] = {*p, '\0'};
                out.AppendUtf8(ch);
            }
            return true;
        }
        found += std::strlen(pattern);
        const char* end = std::strchr(found, '"');
        if (end == nullptr || end > tagClose) {
            return false;
        }
        out.Clear();
        for (const char* p = found; p < end; ++p) {
            const char ch[2] = {*p, '\0'};
            out.AppendUtf8(ch);
        }
        return true;
    }
    return false;
}

const char* FindTag(const char* haystack, const char* tagName) {
    if (haystack == nullptr || tagName == nullptr) {
        return nullptr;
    }
    const std::size_t tagLen = std::strlen(tagName);
    for (const char* p = haystack; *p != '\0'; ++p) {
        if (p[0] == '<' && p[1] != '?' && p[1] != '!' && std::strncmp(p + 1, tagName, tagLen) == 0) {
            const char after = p[1 + tagLen];
            if (after == ' ' || after == '>' || after == '/' || after == '\t' || after == '\n') {
                return p;
            }
        }
    }
    return nullptr;
}

const char* FindCloseTag(const char* from, const char* tagName) {
    if (from == nullptr || tagName == nullptr) {
        return nullptr;
    }
    char close[64]{};
    std::snprintf(close, sizeof(close), "</%s>", tagName);
    return std::strstr(from, close);
}

bool TryGetAttributeUint(const char* tagOpen, const char* tagClose, const char* name, std::uint32_t& out) {
    Utf8String text{};
    if (!TryGetAttribute(tagOpen, tagClose, name, text)) {
        return false;
    }
    const char* c = text.CStr();
    if (c == nullptr || *c == '\0') {
        return false;
    }
    out = static_cast<std::uint32_t>(std::strtoul(c, nullptr, 10));
    return true;
}

bool TryGetAttributeInt(const char* tagOpen, const char* tagClose, const char* name, std::int32_t& out) {
    Utf8String text{};
    if (!TryGetAttribute(tagOpen, tagClose, name, text)) {
        return false;
    }
    const char* c = text.CStr();
    if (c == nullptr || *c == '\0') {
        return false;
    }
    out = static_cast<std::int32_t>(std::strtol(c, nullptr, 10));
    return true;
}

bool TryGetAttributeFloat(const char* tagOpen, const char* tagClose, const char* name, float& out) {
    Utf8String text{};
    if (!TryGetAttribute(tagOpen, tagClose, name, text)) {
        return false;
    }
    const char* c = text.CStr();
    if (c == nullptr || *c == '\0') {
        return false;
    }
    out = std::strtof(c, nullptr);
    return true;
}

void ParsePropertiesBlock(const char* blockStart, const char* blockEnd, TilemapPropertyList& outProps) {
    if (blockStart == nullptr || blockEnd == nullptr) {
        return;
    }
    for (const char* cursor = blockStart; cursor < blockEnd;) {
        const char* propTag = FindTag(cursor, "property");
        if (propTag == nullptr || propTag >= blockEnd) {
            break;
        }
        const char* propClose = std::strchr(propTag, '>');
        if (propClose == nullptr || propClose > blockEnd) {
            break;
        }
        TilemapObjectProperty prop{};
        Utf8String valueAttr{};
        if (TryGetAttribute(propTag, propClose, "name", prop.key)) {
            if (TryGetAttribute(propTag, propClose, "value", valueAttr)) {
                prop.value = valueAttr;
            }
            outProps.PushBack(prop);
        }
        cursor = propClose + 1;
    }
}

bool ParseTilesetTag(
        const char* tagOpen,
        const char* tagClose,
        const char* contentEnd,
        const Utf8String& baseDir,
        TilemapDocumentTileset& outSet,
        Utf8String& outError) {
    const char* openTagEnd = tagClose;
    if (openTagEnd == nullptr || openTagEnd <= tagOpen) {
        openTagEnd = std::strchr(tagOpen, '>');
    }
    if (openTagEnd == nullptr) {
        openTagEnd = contentEnd;
    }

    TryGetAttribute(tagOpen, openTagEnd, "name", outSet.name);
    TryGetAttributeUint(tagOpen, openTagEnd, "tilewidth", outSet.tileWidth);
    TryGetAttributeUint(tagOpen, openTagEnd, "tileheight", outSet.tileHeight);
    TryGetAttributeUint(tagOpen, openTagEnd, "spacing", outSet.spacing);
    TryGetAttributeUint(tagOpen, openTagEnd, "margin", outSet.margin);
    TryGetAttributeUint(tagOpen, openTagEnd, "columns", outSet.columns);
    TryGetAttributeUint(tagOpen, openTagEnd, "tilecount", outSet.tileCount);

    Utf8String sourcePath{};
    if (TryGetAttribute(tagOpen, openTagEnd, "source", sourcePath)) {
        Utf8String tsxPath = JoinPath(baseDir, sourcePath.CStr());
        const Utf8String resolvedTsx = ResolveTilemapAssetPath(tsxPath.CStr());
        if (!resolvedTsx.IsEmpty()) {
            tsxPath = resolvedTsx;
        }
        Array<char> tsxText{};
        if (!ReadFileBytes(tsxPath.CStr(), tsxText)) {
            outError = Utf8String("Failed to read external tileset");
            outError.AppendUtf8(tsxPath.CStr());
            return false;
        }
        const char* tsx = tsxText.GetData();
        const char* tsxSet = FindTag(tsx, "tileset");
        if (tsxSet == nullptr) {
            outError = Utf8String("Missing tileset in TSX");
            return false;
        }
        const char* tsxOpenEnd = std::strchr(tsxSet, '>');
        const char* tsxEnd = FindCloseTag(tsxSet, "tileset");
        if (tsxEnd == nullptr) {
            tsxEnd = tsx + tsxText.GetSize();
        }
        if (!ParseTilesetTag(tsxSet, tsxOpenEnd, tsxEnd, Dirname(tsxPath.CStr()), outSet, outError)) {
            return false;
        }
        return true;
    }

    const char* imgTag = FindTag(tagOpen, "image");
    if (imgTag != nullptr && imgTag < contentEnd) {
        const char* imgClose = std::strchr(imgTag, '>');
        if (imgClose != nullptr) {
            Utf8String relImage{};
            if (TryGetAttribute(imgTag, imgClose, "source", relImage)) {
                const Utf8String joined = JoinPath(baseDir, relImage.CStr());
                const Utf8String resolved = ResolveTilemapAssetPath(joined.CStr());
                outSet.imagePath = resolved.IsEmpty() ? joined : resolved;
            }
            TryGetAttributeUint(imgTag, imgClose, "width", outSet.imageWidth);
            TryGetAttributeUint(imgTag, imgClose, "height", outSet.imageHeight);
        }
    }

    const char* props = FindTag(tagOpen, "properties");
    if (props != nullptr && props < contentEnd) {
        const char* propsEnd = FindCloseTag(props, "properties");
        if (propsEnd != nullptr) {
            ParsePropertiesBlock(props, propsEnd, outSet.properties);
        }
    }
    return true;
}

[[nodiscard]] const TilemapDocumentTileset* FindTilesetForGid(
        const Array<TilemapDocumentTileset>& tilesets,
        const std::uint32_t gid) noexcept {
    const TilemapDocumentTileset* chosen = nullptr;
    for (std::size_t i = 0; i < tilesets.GetSize(); ++i) {
        if (gid < tilesets[i].firstGid) {
            continue;
        }
        if (chosen == nullptr || tilesets[i].firstGid > chosen->firstGid) {
            chosen = &tilesets[i];
        }
    }
    return chosen;
}

TileCell GidToCell(const std::uint32_t rawGid, const Array<TilemapDocumentTileset>& tilesets) {
    std::uint32_t id = 0;
    bool flipH = false;
    bool flipV = false;
    bool flipD = false;
    DecodeTiledGid(rawGid, id, flipH, flipV, flipD);
    if (id == 0U) {
        return TileCell::Empty();
    }
    const TilemapDocumentTileset* set = FindTilesetForGid(tilesets, id);
    if (set == nullptr) {
        return TileCell::Empty();
    }
    const std::uint32_t local = id - set->firstGid;
    if (set->tileCount > 0U && local >= set->tileCount) {
        return TileCell::Empty();
    }
    if (local > 0xFFFEU) {
        return TileCell::Empty();
    }
    const std::uint32_t atlasIndex = TiledLocalTileIndexToSparkAtlasIndex(local, set->columns, set->tileCount);
    TileCell cell = TileCell::FromTileId(static_cast<std::uint16_t>(atlasIndex));
    cell.paintTileId = cell.tileId;
    std::uint8_t rot = flipD ? 1U : 0U;
    bool h = flipH;
    bool v = flipV;
    if (flipD) {
        const bool tmp = h;
        h = !v;
        v = tmp;
    }
    cell.transformFlags = PackTileTransform(h, v, rot);
    return cell;
}

bool ParseCsvLayerData(
        const char* dataStart,
        const char* dataEnd,
        const std::uint32_t width,
        const std::uint32_t height,
        const Array<TilemapDocumentTileset>& tilesets,
        Array<TileCell>& outCells) {
    outCells.Resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    const std::uint32_t total = width * height;
    std::uint32_t index = 0U;
    const char* p = dataStart;
    while (p < dataEnd && index < total) {
        while (p < dataEnd &&
               (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',')) {
            ++p;
        }
        if (p >= dataEnd) {
            break;
        }
        char* endPtr = nullptr;
        const unsigned long value = std::strtoul(p, &endPtr, 10);
        if (endPtr == p) {
            ++p;
            continue;
        }
        const std::uint32_t csvRow = index / width;
        const std::uint32_t x = index % width;
        const std::uint32_t y = height > 0U ? height - 1U - csvRow : 0U;
        const std::size_t dest = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
        outCells[dest] = GidToCell(static_cast<std::uint32_t>(value), tilesets);
        ++index;
        p = endPtr;
    }
    return index == total;
}

bool ParseTileLayer(
        const char* layerTag,
        const char* layerEnd,
        const std::uint32_t mapW,
        const std::uint32_t mapH,
        const Array<TilemapDocumentTileset>& tilesets,
        TilemapDocumentTileLayer& outLayer,
        Utf8String& outError) {
    const char* layerClose = std::strchr(layerTag, '>');
    if (layerClose == nullptr) {
        outError = Utf8String("Malformed layer tag");
        return false;
    }
    TryGetAttribute(layerTag, layerClose, "name", outLayer.name);
    std::int32_t visible = 1;
    if (TryGetAttributeInt(layerTag, layerClose, "visible", visible)) {
        outLayer.visible = visible != 0;
    }

    const char* dataTag = FindTag(layerTag, "data");
    if (dataTag == nullptr || dataTag > layerEnd) {
        outError = Utf8String("Tile layer missing <data>");
        return false;
    }
    const char* dataClose = std::strchr(dataTag, '>');
    if (dataClose == nullptr) {
        return false;
    }
    Utf8String encoding{};
    TryGetAttribute(dataTag, dataClose, "encoding", encoding);
    const char* payload = dataClose + 1;
    const char* dataEnd = FindCloseTag(dataTag, "data");
    if (dataEnd == nullptr) {
        dataEnd = layerEnd;
    }
    if (encoding == Utf8String("csv") || encoding.IsEmpty()) {
        if (!ParseCsvLayerData(payload, dataEnd, mapW, mapH, tilesets, outLayer.cells)) {
            outError = Utf8String("CSV tile layer size mismatch");
            return false;
        }
        return true;
    }
    outError = Utf8String("Unsupported layer data encoding (use csv)");
    return false;
}

void ParseObjectGroup(
        const char* groupTag,
        const char* groupEnd,
        const std::uint32_t mapHeight,
        const std::uint32_t tileWidth,
        const std::uint32_t tileHeight,
        TilemapObjectLayer& outLayer) {
    const char* groupClose = std::strchr(groupTag, '>');
    if (groupClose == nullptr) {
        return;
    }
    TryGetAttribute(groupTag, groupClose, "name", outLayer.name);
    std::int32_t visible = 1;
    if (TryGetAttributeInt(groupTag, groupClose, "visible", visible)) {
        outLayer.visible = visible != 0;
    }

    for (const char* cursor = groupTag; cursor < groupEnd;) {
        const char* objTag = FindTag(cursor, "object");
        if (objTag == nullptr || objTag >= groupEnd) {
            break;
        }
        const char* objClose = std::strchr(objTag, '>');
        if (objClose == nullptr) {
            break;
        }
        TilemapObjectMarker marker{};
        TryGetAttribute(objTag, objClose, "name", marker.name);
        Utf8String typeAttr{};
        if (TryGetAttribute(objTag, objClose, "type", typeAttr)) {
            marker.typeId = typeAttr;
        } else {
            TryGetAttribute(objTag, objClose, "class", marker.typeId);
        }
        float x = 0.0F;
        float y = 0.0F;
        float w = static_cast<float>(tileWidth);
        float h = static_cast<float>(tileHeight);
        TryGetAttributeFloat(objTag, objClose, "x", x);
        TryGetAttributeFloat(objTag, objClose, "y", y);
        TryGetAttributeFloat(objTag, objClose, "width", w);
        TryGetAttributeFloat(objTag, objClose, "height", h);

        const float cellW = static_cast<float>(tileWidth) > 0.0F ? static_cast<float>(tileWidth) : 1.0F;
        const float cellH = static_cast<float>(tileHeight) > 0.0F ? static_cast<float>(tileHeight) : 1.0F;
        marker.cellX = static_cast<std::int32_t>(std::floor(x / cellW));
        const float rowFromTop = std::floor(y / cellH);
        const std::int32_t mapH = static_cast<std::int32_t>(mapHeight > 0U ? mapHeight : 1U);
        marker.cellY = mapH - 1 - static_cast<std::int32_t>(rowFromTop);
        const float xInCell = x - static_cast<float>(marker.cellX) * cellW;
        const float yFromTopInCell = y - rowFromTop * cellH;
        if (w > 0.0F && h > 0.0F) {
            marker.offsetX = (xInCell + w * 0.5F) / cellW;
            marker.offsetY = (cellH - (yFromTopInCell + h * 0.5F)) / cellH;
        } else {
            marker.offsetX = xInCell / cellW;
            marker.offsetY = (cellH - yFromTopInCell) / cellH;
        }

        Utf8String gizmoAttr{};
        if (TryGetAttribute(objTag, objClose, "spark_gizmo", gizmoAttr) && gizmoAttr == Utf8String("1")) {
            marker.mode = TilemapObjectMarkerMode::GizmoOnly;
        }

        const char* props = FindTag(objTag, "properties");
        if (props != nullptr && props < groupEnd) {
            const char* propsEnd = FindCloseTag(props, "properties");
            if (propsEnd != nullptr) {
                ParsePropertiesBlock(props, propsEnd, marker.properties);
            }
        }

        outLayer.markers.PushBack(marker);
        cursor = objClose + 1;
    }
}

}  // namespace

void DecodeTiledGid(
        const std::uint32_t gid,
        std::uint32_t& outTileId,
        bool& outFlipH,
        bool& outFlipV,
        bool& outFlipDiagonal) noexcept {
    outFlipH = (gid & 0x80000000U) != 0U;
    outFlipV = (gid & 0x40000000U) != 0U;
    outFlipDiagonal = (gid & 0x20000000U) != 0U;
    outTileId = gid & 0x1FFFFFFFU;
}

TmxImportResult TmxImporter::ImportFromFile(const char* tmxPath, TilemapDocument& outDocument) const {
    TmxImportResult result{};
    const Utf8String resolvedPath = ResolveTilemapAssetPath(tmxPath);
    if (resolvedPath.IsEmpty()) {
        result.errorMessage = Utf8String("TMX file not found");
        if (tmxPath != nullptr) {
            result.errorMessage.AppendUtf8(tmxPath);
        }
        return result;
    }
    const char* path = resolvedPath.CStr();

    Array<char> fileText{};
    if (!ReadFileBytes(path, fileText)) {
        result.errorMessage = Utf8String("Failed to read TMX");
        result.errorMessage.AppendUtf8(path);
        return result;
    }

    const char* xml = fileText.GetData();
    const char* mapTag = FindTag(xml, "map");
    if (mapTag == nullptr) {
        result.errorMessage = Utf8String("Missing <map> root");
        return result;
    }
    const char* mapClose = FindCloseTag(mapTag, "map");
    if (mapClose == nullptr) {
        result.errorMessage = Utf8String("Unclosed <map>");
        return result;
    }

    outDocument = TilemapDocument{};
    outDocument.sourceTmxPath = Utf8String(path);
    const Utf8String baseDir = Dirname(path);

    const char* mapTagEnd = std::strchr(mapTag, '>');
    if (mapTagEnd == nullptr || mapTagEnd > mapClose) {
        result.errorMessage = Utf8String("Malformed <map> tag");
        return result;
    }

    TryGetAttributeUint(mapTag, mapTagEnd, "width", outDocument.mapWidth);
    TryGetAttributeUint(mapTag, mapTagEnd, "height", outDocument.mapHeight);
    std::uint32_t tilePixelW = 16U;
    std::uint32_t tilePixelH = 16U;
    TryGetAttributeUint(mapTag, mapTagEnd, "tilewidth", tilePixelW);
    TryGetAttributeUint(mapTag, mapTagEnd, "tileheight", tilePixelH);
    /** Derived in <c>ApplyTilemapDocument</c> from <c>pixelsPerWorldUnit</c> when zero. */
    outDocument.tileWorldSize = 0.0F;

    const char* mapProps = FindTag(mapTag, "properties");
    if (mapProps != nullptr && mapProps < mapClose) {
        const char* mapPropsEnd = FindCloseTag(mapProps, "properties");
        if (mapPropsEnd != nullptr) {
            ParsePropertiesBlock(mapProps, mapPropsEnd, outDocument.mapProperties);
        }
    }

    for (const char* cursor = mapTag; cursor < mapClose;) {
        const char* ts = FindTag(cursor, "tileset");
        if (ts == nullptr || ts >= mapClose) {
            break;
        }
        const char* tsEnd = FindCloseTag(ts, "tileset");
        if (tsEnd == nullptr) {
            tsEnd = mapClose;
        }
        TilemapDocumentTileset set{};
        const char* tsOpenEnd = std::strchr(ts, '>');
        TryGetAttributeUint(ts, tsOpenEnd != nullptr ? tsOpenEnd : tsEnd, "firstgid", set.firstGid);
        if (!ParseTilesetTag(ts, tsOpenEnd, tsEnd, baseDir, set, result.errorMessage)) {
            return result;
        }
        if (set.columns == 0U && set.tileCount > 0U) {
            set.columns = set.tileWidth > 0U ? 1U : 1U;
        }
        outDocument.tilesets.PushBack(set);
        if (tsOpenEnd != nullptr && tsOpenEnd < tsEnd) {
            cursor = tsOpenEnd + 1;
        } else {
            cursor = tsEnd + 1;
        }
    }

    if (outDocument.tilesets.IsEmpty()) {
        result.errorMessage = Utf8String("TMX has no tilesets");
        return result;
    }

    std::int32_t layerOrder = 0;
    for (const char* cursor = mapTag; cursor < mapClose;) {
        const char* nextLayer = FindTag(cursor, "layer");
        const char* nextObj = FindTag(cursor, "objectgroup");
        const char* next = nullptr;
        enum class NextKind { Layer, ObjectGroup };
        NextKind kind = NextKind::Layer;
        if (nextLayer == nullptr || nextLayer >= mapClose) {
            next = nextObj;
            kind = NextKind::ObjectGroup;
        } else if (nextObj == nullptr || nextObj >= mapClose) {
            next = nextLayer;
            kind = NextKind::Layer;
        } else if (nextObj < nextLayer) {
            next = nextObj;
            kind = NextKind::ObjectGroup;
        } else {
            next = nextLayer;
            kind = NextKind::Layer;
        }
        if (next == nullptr || next >= mapClose) {
            break;
        }

        if (kind == NextKind::ObjectGroup) {
            const char* groupEnd = FindCloseTag(next, "objectgroup");
            if (groupEnd == nullptr) {
                groupEnd = mapClose;
            }
            TilemapObjectLayer objLayer{};
            ParseObjectGroup(next, groupEnd, outDocument.mapHeight, tilePixelW, tilePixelH, objLayer);
            outDocument.objectLayers.PushBack(objLayer);
            cursor = groupEnd + 1;
            continue;
        }

        const char* layerEnd = FindCloseTag(next, "layer");
        if (layerEnd == nullptr) {
            layerEnd = mapClose;
        }
        TilemapDocumentTileLayer docLayer{};
        docLayer.orderInLayerOffset = layerOrder;
        layerOrder += 4;
        if (!ParseTileLayer(next, layerEnd, outDocument.mapWidth, outDocument.mapHeight, outDocument.tilesets, docLayer,
                    result.errorMessage)) {
            return result;
        }
        outDocument.tileLayers.PushBack(docLayer);
        cursor = layerEnd + 1;
    }

    result.success = true;
    return result;
}

}  // namespace Spark
