#include "spark/scene/serialization/TilemapComponentSnapshot.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/serialization/ComponentSnapshotPayload.hpp"
#include "spark/scene/tilemap/Tileset.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/tilemap/TileCell.hpp"
#include "spark/scene/tilemap/TilemapLayer.hpp"

#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

void AppendCellRecord(Utf8String& out, const std::uint32_t x, const std::uint32_t y, const TileCell& cell) {
    char buf[128]{};
    std::snprintf(
            buf,
            sizeof(buf),
            "%u %u %u %u %u %u %u %u %u ",
            x,
            y,
            static_cast<unsigned>(cell.tileId),
            static_cast<unsigned>(cell.paintTileId),
            static_cast<unsigned>(cell.transformFlags),
            static_cast<unsigned>(cell.tintR),
            static_cast<unsigned>(cell.tintG),
            static_cast<unsigned>(cell.tintB),
            static_cast<unsigned>(cell.tintA));
    out.AppendUtf8(buf);
}

[[nodiscard]] Utf8String ResolveTilesetTextureKey(
        const TilemapComponent& tilemap,
        const GameObject& owner,
        const SceneCaptureContext& ctx) {
    if (ctx.resolveTexturePath != nullptr) {
        const Utf8String resolved = ctx.resolveTexturePath(owner, ctx.textureUserData);
        if (!resolved.IsEmpty()) {
            return resolved;
        }
    }
    if (const SharedPtr<Texture2D>& atlas = tilemap.GetAtlas()) {
        return atlas->GetName();
    }
    return {};
}

[[nodiscard]] SharedPtr<Texture2D> ResolveTilesetTexture(
        const char* textureKey,
        GameWorld& world,
        const SceneApplyContext& ctx) {
    if (textureKey == nullptr || textureKey[0] == '\0') {
        return SharedPtr<Texture2D>();
    }
    if (SharedPtr<Texture2D> cached = world.TryGetTextureByKeyOrPath(textureKey)) {
        return cached;
    }
    if (ctx.assetsRoot != nullptr) {
        char joined[1024]{};
        std::snprintf(joined, sizeof(joined), "%s/%s", ctx.assetsRoot, textureKey);
        if (SharedPtr<Texture2D> loaded = world.LoadTexture(joined)) {
            return loaded;
        }
    }
    return world.LoadTexture(textureKey);
}

}  // namespace

bool TilemapComponentSnapshot::TryCapture(
        const TilemapComponent& tilemap,
        const GameObject& owner,
        const SceneCaptureContext& ctx,
        Utf8String& outPayload) {
    if (tilemap.GetMapWidth() == 0U || tilemap.GetMapHeight() == 0U) {
        return false;
    }
    const Utf8String textureKey = ResolveTilesetTextureKey(tilemap, owner, ctx);

    Utf8String payload;
    payload.AppendUtf8("v1 ");
    ComponentSnapshotPayload::AppendQuotedString(payload, textureKey.CStr());
    char header[192]{};
    std::snprintf(
            header,
            sizeof(header),
            " %u %u %u %u %.6f %d %u ",
            tilemap.GetAtlasTilesU(),
            tilemap.GetAtlasTilesV(),
            tilemap.GetMapWidth(),
            tilemap.GetMapHeight(),
            tilemap.GetTileWorldSize(),
            tilemap.GetSortOrderBase(),
            tilemap.GetLayerCount());
    payload.AppendUtf8(header);

    for (std::uint32_t layerIndex = 0; layerIndex < tilemap.GetLayerCount(); ++layerIndex) {
        const TilemapLayer& layer = tilemap.GetLayer(layerIndex);
        ComponentSnapshotPayload::AppendQuotedString(payload, layer.name.CStr());
        std::size_t sparseCount = 0;
        Utf8String sparsePayload;
        const std::uint32_t mapW = tilemap.GetMapWidth();
        const std::uint32_t mapH = tilemap.GetMapHeight();
        for (std::uint32_t y = 0; y < mapH; ++y) {
            for (std::uint32_t x = 0; x < mapW; ++x) {
                const TileCell cell = tilemap.GetTileCell(layerIndex, x, y);
                if (cell.IsEmpty()) {
                    continue;
                }
                ++sparseCount;
                AppendCellRecord(sparsePayload, x, y, cell);
            }
        }
        char layerHeader[128]{};
        std::snprintf(
                layerHeader,
                sizeof(layerHeader),
                " %d %u %u %u %u %zu ",
                layer.orderInLayerOffset,
                layer.visible ? 1U : 0U,
                layer.contributeCollision ? 1U : 0U,
                layer.contributeGameplayGrid ? 1U : 0U,
                static_cast<unsigned>(layer.sortMode),
                sparseCount);
        payload.AppendUtf8(layerHeader);
        payload.AppendUtf8(sparsePayload);
    }

    outPayload = MoveTemp(payload);
    return true;
}

bool TilemapComponentSnapshot::TryRestore(
        GameObject& owner,
        const char* payload,
        GameWorld& world,
        const SceneApplyContext& ctx) {
    if (payload == nullptr || std::strncmp(payload, "v1 ", 3) != 0) {
        return false;
    }
    const char* cursor = payload + 3;
    char textureKey[384]{};
    if (!ComponentSnapshotPayload::ParseLeadingQuotedString(cursor, textureKey, sizeof(textureKey))) {
        return false;
    }
    unsigned tilesU = 8;
    unsigned tilesV = 8;
    unsigned mapW = 0;
    unsigned mapH = 0;
    float tileWorldSize = 1.0F;
    int sortOrder = 0;
    unsigned layerCount = 0;
    if (std::sscanf(cursor, "%u %u %u %u %f %d %u", &tilesU, &tilesV, &mapW, &mapH, &tileWorldSize, &sortOrder, &layerCount) <
        7) {
        return false;
    }
    ComponentSnapshotPayload::SkipTokens(cursor, 7);

    SharedPtr<Texture2D> atlas = ResolveTilesetTexture(textureKey, world, ctx);
    if (!atlas) {
        Texture2D placeholder{};
        placeholder.GetName() = Utf8String(textureKey);
        atlas = world.RegisterTexture(MakeShared<Texture2D>(MoveTemp(placeholder)), textureKey);
    }

    TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (tilemap == nullptr) {
        tilemap = owner.AddComponent<TilemapComponent>(
                MakeShared<Tileset>(atlas, tilesU, tilesV), mapW, mapH, tileWorldSize, sortOrder);
    } else {
        tilemap->SetTileset(MakeShared<Tileset>(atlas, tilesU, tilesV));
        tilemap->Resize(mapW, mapH);
        tilemap->SetTileWorldSize(tileWorldSize);
        tilemap->SetSortOrderBase(sortOrder);
    }

    while (tilemap->GetLayerCount() > 1U) {
        tilemap->RemoveLayer(tilemap->GetLayerCount() - 1U);
    }
    while (tilemap->GetLayerCount() < layerCount) {
        tilemap->AddLayer();
    }

    for (std::uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
        while (*cursor == ' ') {
            ++cursor;
        }
        char layerName[128]{};
        if (!ComponentSnapshotPayload::ParseLeadingQuotedString(cursor, layerName, sizeof(layerName))) {
            return false;
        }
        int orderOffset = 0;
        unsigned visible = 1;
        unsigned contributeCollision = 1;
        unsigned contributeGameplayGrid = 1;
        unsigned sortMode = 0;
        std::size_t sparseCount = 0;
        if (std::sscanf(
                    cursor,
                    "%d %u %u %u %u %zu",
                    &orderOffset,
                    &visible,
                    &contributeCollision,
                    &contributeGameplayGrid,
                    &sortMode,
                    &sparseCount) < 6) {
            return false;
        }
        ComponentSnapshotPayload::SkipTokens(cursor, 6);

        TilemapLayer& layer = tilemap->GetLayer(layerIndex);
        layer.name = Utf8String(layerName);
        layer.orderInLayerOffset = orderOffset;
        layer.visible = visible != 0U;
        layer.contributeCollision = contributeCollision != 0U;
        layer.contributeGameplayGrid = contributeGameplayGrid != 0U;
        layer.sortMode = static_cast<TilemapLayerSortMode>(sortMode);
        layer.cells.Resize(static_cast<std::size_t>(mapW) * static_cast<std::size_t>(mapH));
        for (std::size_t i = 0; i < layer.cells.GetSize(); ++i) {
            layer.cells[i] = TileCell::Empty();
        }

        for (std::size_t si = 0; si < sparseCount; ++si) {
            unsigned x = 0;
            unsigned y = 0;
            unsigned tileId = 0;
            unsigned paintId = TileCell::kEmptyTileId;
            unsigned transformFlags = 0;
            unsigned tr = 255;
            unsigned tg = 255;
            unsigned tb = 255;
            unsigned ta = 255;
            if (std::sscanf(
                        cursor,
                        "%u %u %u %u %u %u %u %u %u",
                        &x,
                        &y,
                        &tileId,
                        &paintId,
                        &transformFlags,
                        &tr,
                        &tg,
                        &tb,
                        &ta) < 9) {
                return false;
            }
            ComponentSnapshotPayload::SkipTokens(cursor, 9);
            TileCell cell{};
            cell.tileId = static_cast<std::uint16_t>(tileId);
            cell.paintTileId = static_cast<std::uint16_t>(paintId);
            cell.transformFlags = static_cast<std::uint8_t>(transformFlags);
            cell.tintR = static_cast<std::uint8_t>(tr);
            cell.tintG = static_cast<std::uint8_t>(tg);
            cell.tintB = static_cast<std::uint8_t>(tb);
            cell.tintA = static_cast<std::uint8_t>(ta);
            tilemap->SetTileCell(layerIndex, x, y, cell);
        }
    }

    return true;
}

}  // namespace Spark
