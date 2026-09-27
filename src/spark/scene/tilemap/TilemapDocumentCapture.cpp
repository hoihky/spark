#include "spark/scene/tilemap/TilemapDocumentCapture.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/tilemap/Tileset.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

namespace {

void AppendPrimaryTileset(const TilemapComponent& tilemap, TilemapDocument& document) noexcept {
    document.tilesets.Clear();
    const SharedPtr<Tileset>& tileset = tilemap.GetTileset();
    if (!tileset) {
        return;
    }

    TilemapDocumentTileset set{};
    set.firstGid = 1U;
    set.name = Utf8String("Tileset");
    set.columns = tileset->GetTilesU() > 0U ? tileset->GetTilesU() : 1U;
    set.tileCount = tileset->GetCellCount();
    set.margin = static_cast<std::uint32_t>(tileset->GetMarginPixels());
    set.spacing = static_cast<std::uint32_t>(tileset->GetSpacingPixels());
    set.tileWidth = tileset->GetTilePixelWidth();
    set.tileHeight = tileset->GetTilePixelHeight();
    set.imageWidth = tileset->GetImagePixelWidth();
    set.imageHeight = tileset->GetImagePixelHeight();
    if (set.tileWidth == 0U && tileset->GetAtlas()) {
        const std::uint32_t cols = set.columns > 0U ? set.columns : 1U;
        set.tileWidth = tileset->GetAtlas()->GetWidth() / cols;
    }
    if (set.tileHeight == 0U && tileset->GetAtlas()) {
        const std::uint32_t rows =
                set.columns > 0U ? (set.tileCount + set.columns - 1U) / set.columns : tileset->GetTilesV();
        if (rows > 0U) {
            set.tileHeight = tileset->GetAtlas()->GetHeight() / rows;
        }
    }
    if (tileset->GetAtlas()) {
        set.imagePath = tileset->GetAtlas()->GetName();
    }
    document.tilesets.PushBack(set);
}

void AppendTileLayers(const TilemapComponent& tilemap, TilemapDocument& document) noexcept {
    document.tileLayers.Clear();
    const std::uint32_t layerCount = tilemap.GetLayerCount();
    for (std::uint32_t li = 0; li < layerCount; ++li) {
        const TilemapLayer& src = tilemap.GetLayer(li);
        TilemapDocumentTileLayer dst{};
        dst.name = src.name;
        dst.visible = src.visible;
        dst.orderInLayerOffset = src.orderInLayerOffset;
        dst.contributeCollision = src.contributeCollision;
        dst.contributeGameplayGrid = src.contributeGameplayGrid;
        dst.sortMode = src.sortMode;
        dst.cells = src.cells;
        document.tileLayers.PushBack(dst);
    }
}

void AppendObjectLayers(const TilemapObjectLayerComponent& objects, TilemapDocument& document) noexcept {
    document.objectLayers.Clear();
    const std::uint32_t count = objects.GetLayerCount();
    for (std::uint32_t li = 0; li < count; ++li) {
        document.objectLayers.PushBack(objects.GetLayer(li));
    }
}

}  // namespace

TilemapDocumentCapturer::Result TilemapDocumentCapturer::Capture(
        const GameObject& owner,
        const TilemapComponent& tilemap) const noexcept {
    return Capture(owner, tilemap, Options{});
}

TilemapDocumentCapturer::Result TilemapDocumentCapturer::Capture(
        const GameObject& owner,
        const TilemapComponent& tilemap,
        const Options& options) const noexcept {
    Result result{};
    if (tilemap.GetMapWidth() == 0U || tilemap.GetMapHeight() == 0U) {
        result.errorMessage = Utf8String("Tilemap has zero size");
        return result;
    }
    if (tilemap.GetLayerCount() == 0U) {
        result.errorMessage = Utf8String("Tilemap has no layers");
        return result;
    }

    TilemapDocument& document = result.document;
    document.mapWidth = tilemap.GetMapWidth();
    document.mapHeight = tilemap.GetMapHeight();
    document.tileWorldSize = tilemap.GetTileWorldSize();
    document.sortOrderBase = tilemap.GetSortOrderBase();

    if (!options.sourceTmxPathOverride.IsEmpty()) {
        document.sourceTmxPath = options.sourceTmxPathOverride;
    } else if (const TilemapMapSourceComponent* source = owner.GetComponent<TilemapMapSourceComponent>()) {
        if (!source->GetTmxPath().IsEmpty()) {
            document.sourceTmxPath = source->GetTmxPath();
        } else if (!source->GetSparkMapPath().IsEmpty()) {
            document.sourceTmxPath = source->GetSparkMapPath();
        }
    }

    AppendPrimaryTileset(tilemap, document);
    AppendTileLayers(tilemap, document);

    if (options.includeObjectLayers) {
        if (const TilemapObjectLayerComponent* objects = owner.GetComponent<TilemapObjectLayerComponent>()) {
            AppendObjectLayers(*objects, document);
        }
    }

    result.success = true;
    return result;
}

TilemapDocumentCapturer::Result TilemapDocumentCapturer::CaptureFromOwner(const GameObject& owner) const noexcept {
    return CaptureFromOwner(owner, Options{});
}

TilemapDocumentCapturer::Result TilemapDocumentCapturer::CaptureFromOwner(
        const GameObject& owner,
        const Options& options) const noexcept {
    Result result{};
    const TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (tilemap == nullptr) {
        result.errorMessage = Utf8String("Owner has no TilemapComponent");
        return result;
    }
    return Capture(owner, *tilemap, options);
}

}  // namespace Spark
