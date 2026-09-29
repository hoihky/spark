#include "spark/scene/tilemap/TileGameplayProperties.hpp"

#include "spark/scene/tilemap/TileDefinition.hpp"
#include "spark/scene/tilemap/TileTransform.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] bool PropertyEquals(const TilemapObjectProperty& prop, const char* key) noexcept {
    return key != nullptr && prop.key == Utf8String(key);
}

[[nodiscard]] bool ParseBool(const char* value, const bool defaultValue) noexcept {
    if (value == nullptr) {
        return defaultValue;
    }
    if (std::strcmp(value, "true") == 0 || std::strcmp(value, "1") == 0) {
        return true;
    }
    if (std::strcmp(value, "false") == 0 || std::strcmp(value, "0") == 0) {
        return false;
    }
    return defaultValue;
}

}  // namespace

bool TilemapPropertyListGetBool(
        const TilemapPropertyList& properties,
        const char* key,
        const bool defaultValue) noexcept {
    for (std::size_t i = 0; i < properties.GetSize(); ++i) {
        if (PropertyEquals(properties[i], key)) {
            return ParseBool(properties[i].value.CStr(), defaultValue);
        }
    }
    return defaultValue;
}

const char* TilemapPropertyListGetString(
        const TilemapPropertyList& properties,
        const char* key,
        const char* defaultValue) noexcept {
    for (std::size_t i = 0; i < properties.GetSize(); ++i) {
        if (PropertyEquals(properties[i], key)) {
            return properties[i].value.CStr();
        }
    }
    return defaultValue;
}

void ApplyTileGameplayPropertiesToDefinition(
        TileDefinition& definition,
        const TilemapPropertyList& properties) noexcept {
    if (properties.IsEmpty()) {
        return;
    }

    const char* collision = TilemapPropertyListGetString(
            properties, TileGameplayPropertyNames::kCollision, nullptr);
    if (collision != nullptr) {
        if (std::strcmp(collision, "none") == 0) {
            definition.collisionShape = TileCollisionShape::None;
        } else if (std::strcmp(collision, "full") == 0) {
            definition.collisionShape = TileCollisionShape::FullCell;
        } else if (std::strcmp(collision, "bottom_half") == 0) {
            definition.collisionShape = TileCollisionShape::BottomHalf;
        } else if (std::strcmp(collision, "top_half") == 0) {
            definition.collisionShape = TileCollisionShape::TopHalf;
        }
    }

    const bool walkable =
            TilemapPropertyListGetBool(properties, TileGameplayPropertyNames::kWalkable, false);
    if (walkable) {
        definition.collisionShape = TileCollisionShape::None;
        definition.flags = TileDefinitionFlags::ForceWalkable;
    }

    if (TilemapPropertyListGetBool(properties, TileGameplayPropertyNames::kBlocksPathfinding, false)) {
        definition.flags = definition.flags | TileDefinitionFlags::BlocksPathfinding;
    }
    if (walkable) {
        definition.flags = static_cast<TileDefinitionFlags>(
                static_cast<std::uint16_t>(definition.flags) &
                ~static_cast<std::uint16_t>(TileDefinitionFlags::BlocksPathfinding));
    }
}

void ApplyTileGameplayPropertiesFromTilesetDocument(
        Tileset& tileset,
        const TilemapDocumentTileset& documentTileset) noexcept {
    if (documentTileset.perTileProperties.IsEmpty()) {
        return;
    }
    tileset.EnsureDefinitions();
    const std::uint32_t columns = documentTileset.columns > 0U ? documentTileset.columns : 1U;
    const std::uint32_t tileCount =
            documentTileset.tileCount > 0U ? documentTileset.tileCount : tileset.GetCellCount();

    for (std::size_t i = 0; i < documentTileset.perTileProperties.GetSize(); ++i) {
        const TilemapDocumentPerTileProperties& entry = documentTileset.perTileProperties[i];
        const std::uint32_t atlasIndex = TiledLocalTileIndexToSparkAtlasIndex(
                entry.localTileId, columns, tileCount);
        if (atlasIndex >= tileset.GetCellCount()) {
            continue;
        }
        ApplyTileGameplayPropertiesToDefinition(
                tileset.Definition(static_cast<std::uint16_t>(atlasIndex)), entry.properties);
    }
}

}  // namespace Spark
