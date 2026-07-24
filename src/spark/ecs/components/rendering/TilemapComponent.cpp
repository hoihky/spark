#include "spark/ecs/components/rendering/TilemapComponent.hpp"

#include "spark/scene/tilemap/TilemapGameplayGridBake.hpp"

namespace Spark {

namespace {

void ClearCellsToEmpty(Array<TileCell>& outCells) {
    for (std::size_t i = 0; i < outCells.GetSize(); ++i) {
        outCells[i] = TileCell::Empty();
    }
}

void ResizeLayerCells(TilemapLayer& layer, const std::size_t cellCount) {
    layer.cells.Resize(cellCount);
    ClearCellsToEmpty(layer.cells);
}

}  // namespace

TilemapComponent::TilemapComponent(
        SharedPtr<Texture2D> inAtlas,
        const std::uint32_t mapW,
        const std::uint32_t mapH,
        const std::uint32_t inAtlasTilesU,
        const std::uint32_t inAtlasTilesV,
        const float inTileWorldSize,
        const std::int32_t inSortOrderBase) noexcept
        : tileset(CreateTilesetFromAtlas(MoveTemp(inAtlas), inAtlasTilesU, inAtlasTilesV)),
          tileWorldSize(inTileWorldSize > 0.0F ? inTileWorldSize : 1.0F),
          sortOrderBase(inSortOrderBase) {
    Resize(mapW, mapH);
}

TilemapComponent::TilemapComponent(
        SharedPtr<Tileset> inTileset,
        const std::uint32_t mapW,
        const std::uint32_t mapH,
        const float inTileWorldSize,
        const std::int32_t inSortOrderBase) noexcept
        : tileset(MoveTemp(inTileset)),
          tileWorldSize(inTileWorldSize > 0.0F ? inTileWorldSize : 1.0F),
          sortOrderBase(inSortOrderBase) {
    Resize(mapW, mapH);
}

void TilemapComponent::SetTileset(SharedPtr<Tileset> inTileset) noexcept {
    tileset = MoveTemp(inTileset);
    if (tileset) {
        tileset->EnsureDefinitions();
    }
}

const SharedPtr<Texture2D>& TilemapComponent::GetAtlas() const noexcept {
    static const SharedPtr<Texture2D> kEmpty{};
    if (tileset && tileset->GetAtlas()) {
        return tileset->GetAtlas();
    }
    return kEmpty;
}

std::uint32_t TilemapComponent::GetAtlasTilesU() const noexcept {
    return tileset ? tileset->GetTilesU() : 1U;
}

std::uint32_t TilemapComponent::GetAtlasTilesV() const noexcept {
    return tileset ? tileset->GetTilesV() : 1U;
}

void TilemapComponent::EnsureDefaultLayer() noexcept {
    if (layers.IsEmpty()) {
        layers.PushBack(TilemapLayer::MakeDefault());
    }
}

void TilemapComponent::Resize(const std::uint32_t mapW, const std::uint32_t mapH) {
    mapWidth = mapW;
    mapHeight = mapH;
    EnsureDefaultLayer();

    const std::uint64_t n = static_cast<std::uint64_t>(mapW) * static_cast<std::uint64_t>(mapH);
    if (n == 0 || n > 1ULL << 24) {
        for (std::size_t li = 0; li < layers.GetSize(); ++li) {
            layers[li].cells.Clear();
        }
        return;
    }
    const std::size_t cellCount = static_cast<std::size_t>(n);
    for (std::size_t li = 0; li < layers.GetSize(); ++li) {
        ResizeLayerCells(layers[li], cellCount);
    }
}

TilemapLayer& TilemapComponent::GetLayer(const std::uint32_t layerIndex) noexcept {
    EnsureDefaultLayer();
    if (layerIndex >= layers.GetSize()) {
        return layers[0];
    }
    return layers[static_cast<std::size_t>(layerIndex)];
}

const TilemapLayer& TilemapComponent::GetLayer(const std::uint32_t layerIndex) const noexcept {
    static const TilemapLayer kEmptyLayer{};
    if (layerIndex >= layers.GetSize()) {
        return kEmptyLayer;
    }
    return layers[static_cast<std::size_t>(layerIndex)];
}

std::uint32_t TilemapComponent::AddLayer(const char* name) {
    EnsureDefaultLayer();
    TilemapLayer layer{};
    layer.name = Utf8String(name != nullptr ? name : "Layer");
    const std::uint64_t n = static_cast<std::uint64_t>(mapWidth) * static_cast<std::uint64_t>(mapHeight);
    if (n > 0 && n <= 1ULL << 24) {
        ResizeLayerCells(layer, static_cast<std::size_t>(n));
    }
    layers.PushBack(MoveTemp(layer));
    return static_cast<std::uint32_t>(layers.GetSize() - 1U);
}

void TilemapComponent::RemoveLayer(const std::uint32_t layerIndex) noexcept {
    if (layers.GetSize() <= 1U || layerIndex >= layers.GetSize()) {
        return;
    }
    layers.RemoveAt(static_cast<std::size_t>(layerIndex));
}

bool TilemapComponent::TryCellIndex(
        const std::uint32_t layerIndex,
        const std::uint32_t x,
        const std::uint32_t y,
        std::size_t& outIndex) const noexcept {
    if (layerIndex >= layers.GetSize() || x >= mapWidth || y >= mapHeight) {
        return false;
    }
    const Array<TileCell>& cells = layers[static_cast<std::size_t>(layerIndex)].cells;
    if (cells.GetSize() == 0) {
        return false;
    }
    outIndex = static_cast<std::size_t>(y) * static_cast<std::size_t>(mapWidth) + static_cast<std::size_t>(x);
    return outIndex < cells.GetSize();
}

void TilemapComponent::SetTile(const std::uint32_t x, const std::uint32_t y, const std::uint16_t tileId) {
    SetTile(kDefaultLayerIndex, x, y, tileId);
}

std::uint16_t TilemapComponent::GetTile(const std::uint32_t x, const std::uint32_t y) const noexcept {
    return GetTileCell(x, y).tileId;
}

void TilemapComponent::SetTileCell(const std::uint32_t x, const std::uint32_t y, const TileCell& cell) {
    SetTileCell(kDefaultLayerIndex, x, y, cell);
}

TileCell TilemapComponent::GetTileCell(const std::uint32_t x, const std::uint32_t y) const noexcept {
    return GetTileCell(kDefaultLayerIndex, x, y);
}

void TilemapComponent::SetTile(
        const std::uint32_t layerIndex,
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint16_t tileId) {
    SetTileCell(layerIndex, x, y, TileCell::FromTileId(tileId));
}

void TilemapComponent::SetTileCell(
        const std::uint32_t layerIndex,
        const std::uint32_t x,
        const std::uint32_t y,
        const TileCell& cell) {
    std::size_t index = 0;
    if (!TryCellIndex(layerIndex, x, y, index)) {
        return;
    }
    layers[static_cast<std::size_t>(layerIndex)].cells[index] = cell;
}

TileCell TilemapComponent::GetTileCell(
        const std::uint32_t layerIndex,
        const std::uint32_t x,
        const std::uint32_t y) const noexcept {
    std::size_t index = 0;
    if (!TryCellIndex(layerIndex, x, y, index)) {
        return TileCell::Empty();
    }
    return layers[static_cast<std::size_t>(layerIndex)].cells[index];
}

const TileDefinition& TilemapComponent::GetTileDefinition(const std::uint32_t x, const std::uint32_t y) const noexcept {
    return GetTileDefinition(kDefaultLayerIndex, x, y);
}

const TileDefinition& TilemapComponent::GetTileDefinition(
        const std::uint32_t layerIndex,
        const std::uint32_t x,
        const std::uint32_t y) const noexcept {
    const TileCell cell = GetTileCell(layerIndex, x, y);
    return GetDefinitionForTileId(cell.tileId);
}

const TileDefinition& TilemapComponent::GetPaintTileDefinition(
        const std::uint32_t layerIndex,
        const std::uint32_t x,
        const std::uint32_t y) const noexcept {
    const TileCell cell = GetTileCell(layerIndex, x, y);
    return GetDefinitionForTileId(cell.GetPaintTileId());
}

const TileDefinition& TilemapComponent::GetDefinitionForTileId(const std::uint16_t tileId) const noexcept {
    if (tileId == TileCell::kEmptyTileId || !tileset) {
        static const TileDefinition kEmptyDef{};
        return kEmptyDef;
    }
    return tileset->Definition(tileId);
}

void TilemapComponent::SetPaintTile(const std::uint32_t x, const std::uint32_t y, const std::uint16_t paintTileId) {
    SetPaintTile(kDefaultLayerIndex, x, y, paintTileId);
}

void TilemapComponent::SetPaintTile(
        const std::uint32_t layerIndex,
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint16_t paintTileId) {
    TileCell cell = TileCell::FromTileId(paintTileId);
    cell.paintTileId = paintTileId;
    SetTileCell(layerIndex, x, y, cell);
}

void TilemapComponent::BakeGameplayGrid(
        TilemapGameplayGrid& outGrid,
        const TilemapGameplayWalkRule rule) const noexcept {
    BakeTilemapGameplayGrid(*this, rule, outGrid);
}

}  // namespace Spark
