#include "spark/scene/tilemap/TilemapEditSession.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/tilemap/TilemapDerivedDataRebake.hpp"
#include "spark/scene/tilemap/TilemapDocumentCapture.hpp"
#include "spark/scene/tilemap/TilemapLayer.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Spark {

namespace {

[[nodiscard]] std::size_t CellIndex(
        const std::uint32_t cellX,
        const std::uint32_t cellY,
        const std::uint32_t mapWidth) noexcept {
    return static_cast<std::size_t>(cellY) * static_cast<std::size_t>(mapWidth) + static_cast<std::size_t>(cellX);
}

[[nodiscard]] bool CellsEqual(const TileCell& a, const TileCell& b) noexcept {
    return a.tileId == b.tileId && a.paintTileId == b.paintTileId && a.transformFlags == b.transformFlags &&
           a.tintR == b.tintR && a.tintG == b.tintG && a.tintB == b.tintB && a.tintA == b.tintA;
}

void ResizeDocumentLayers(TilemapDocument& document, const std::uint32_t newWidth, const std::uint32_t newHeight) noexcept {
    const std::uint32_t oldWidth = document.mapWidth;
    const std::uint32_t oldHeight = document.mapHeight;
    document.mapWidth = newWidth;
    document.mapHeight = newHeight;

    if (newWidth == 0U || newHeight == 0U) {
        for (std::size_t li = 0; li < document.tileLayers.GetSize(); ++li) {
            document.tileLayers[li].cells.Clear();
        }
        return;
    }

    const std::size_t newCount = static_cast<std::size_t>(newWidth) * static_cast<std::size_t>(newHeight);
    const std::uint32_t copyWidth = std::min(oldWidth, newWidth);
    const std::uint32_t copyHeight = std::min(oldHeight, newHeight);

    for (std::size_t li = 0; li < document.tileLayers.GetSize(); ++li) {
        const Array<TileCell>& oldCells = document.tileLayers[li].cells;
        Array<TileCell> newCells{};
        newCells.Resize(newCount);
        for (std::size_t i = 0; i < newCount; ++i) {
            newCells[i] = TileCell::Empty();
        }
        if (oldWidth > 0U && oldHeight > 0U) {
            for (std::uint32_t y = 0; y < copyHeight; ++y) {
                for (std::uint32_t x = 0; x < copyWidth; ++x) {
                    const std::size_t oldIndex = CellIndex(x, y, oldWidth);
                    const std::size_t newIndex = CellIndex(x, y, newWidth);
                    if (oldIndex < oldCells.GetSize() && newIndex < newCount) {
                        newCells[newIndex] = oldCells[oldIndex];
                    }
                }
            }
        }
        document.tileLayers[li].cells = MoveTemp(newCells);
    }
}

void CollectLineCells(
        const std::int32_t x0,
        const std::int32_t y0,
        const std::int32_t x1,
        const std::int32_t y1,
        Array<std::pair<std::uint32_t, std::uint32_t>>& outCells) {
    outCells.Clear();
    std::int32_t x = x0;
    std::int32_t y = y0;
    const std::int32_t dx = std::abs(x1 - x0);
    const std::int32_t sx = x0 < x1 ? 1 : -1;
    const std::int32_t dy = -std::abs(y1 - y0);
    const std::int32_t sy = y0 < y1 ? 1 : -1;
    std::int32_t err = dx + dy;

    while (true) {
        if (x >= 0 && y >= 0) {
            outCells.PushBack({static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y)});
        }
        if (x == x1 && y == y1) {
            break;
        }
        const std::int32_t err2 = 2 * err;
        if (err2 >= dy) {
            err += dy;
            x += sx;
        }
        if (err2 <= dx) {
            err += dx;
            y += sy;
        }
    }
}

}  // namespace

bool TilemapEditSession::Attach(GameObject& inOwner) noexcept {
    TilemapEditSessionOptions defaultOptions{};
    return Attach(inOwner, defaultOptions);
}

bool TilemapEditSession::Attach(GameObject& inOwner, const TilemapEditSessionOptions& options) noexcept {
    Detach();
    const TilemapDocumentCapturer::Result captured = documentCapturer_.CaptureFromOwner(inOwner);
    if (!captured.IsSuccess()) {
        return false;
    }
    TilemapComponent* component = inOwner.GetComponent<TilemapComponent>();
    if (component == nullptr) {
        return false;
    }
    owner = &inOwner;
    tilemap = component;
    document = captured.GetDocument();
    activeLayerIndex = options.activeLayerIndex;
    maxUndoSteps = options.maxUndoSteps > 0U ? options.maxUndoSteps : 64U;
    autoRebakePolicy = options.autoRebakePolicy;
    validateAfterCommit = options.validateAfterCommit;
    derivedRebakeOptions = options.derivedRebakeOptions;
    lastValidationSummary.Clear();
    brush = {};
    revisionTracker.Reset();
    ClearHistory();
    return true;
}

void TilemapEditSession::SetDerivedRebakeContext(GameWorld* worldForPhysics, PhysicsSubsystem* physics) noexcept {
    rebakeWorld = worldForPhysics;
    rebakePhysics = physics;
}

void TilemapEditSession::Detach() noexcept {
    if (gestureOpen) {
        EndGesture();
    }
    owner = nullptr;
    tilemap = nullptr;
    document = {};
    ClearHistory();
}

bool TilemapEditSession::InBounds(const std::uint32_t cellX, const std::uint32_t cellY) const noexcept {
    return cellX < document.mapWidth && cellY < document.mapHeight;
}

TileCell TilemapEditSession::DocumentCellConst(
        const std::uint32_t layerIndex,
        const std::uint32_t cellX,
        const std::uint32_t cellY) const noexcept {
    if (layerIndex >= document.tileLayers.GetSize() || !InBounds(cellX, cellY)) {
        return TileCell::Empty();
    }
    const Array<TileCell>& cells = document.tileLayers[layerIndex].cells;
    const std::size_t index = CellIndex(cellX, cellY, document.mapWidth);
    if (index >= cells.GetSize()) {
        return TileCell::Empty();
    }
    return cells[index];
}

TileCell& TilemapEditSession::DocumentCell(
        const std::uint32_t layerIndex,
        const std::uint32_t cellX,
        const std::uint32_t cellY) noexcept {
    static TileCell kDiscard{};
    if (layerIndex >= document.tileLayers.GetSize() || !InBounds(cellX, cellY)) {
        return kDiscard;
    }
    Array<TileCell>& cells = document.tileLayers[layerIndex].cells;
    const std::size_t index = CellIndex(cellX, cellY, document.mapWidth);
    if (index >= cells.GetSize()) {
        return kDiscard;
    }
    return cells[index];
}

void TilemapEditSession::RecordCellMutation(
        const std::uint32_t layerIndex,
        const std::uint32_t cellX,
        const std::uint32_t cellY,
        const TileCell& before,
        const TileCell& after) noexcept {
    if (CellsEqual(before, after)) {
        return;
    }
    for (std::size_t i = 0; i < pendingEdits.GetSize(); ++i) {
        TilemapCellEdit& edit = pendingEdits[i];
        if (edit.layerIndex == layerIndex && edit.cellX == cellX && edit.cellY == cellY) {
            edit.after = after;
            return;
        }
    }
    TilemapCellEdit edit{};
    edit.layerIndex = layerIndex;
    edit.cellX = cellX;
    edit.cellY = cellY;
    edit.before = before;
    edit.after = after;
    pendingEdits.PushBack(edit);

    if (!gestureOpen) {
        CommitPendingEdits(TilemapEditChangeKind::CellPaint);
    }
}

void TilemapEditSession::CommitPendingEdits(const TilemapEditChangeKind changeKind) noexcept {
    if (pendingEdits.IsEmpty()) {
        return;
    }
    TilemapEditCommand command{};
    command.kind = TilemapEditCommandKind::CellEdits;
    command.changeKind = changeKind;
    command.cellEdits = pendingEdits;
    pendingEdits.Clear();
    BumpRevisionForEdits(command.cellEdits, changeKind);
    PushUndoCommand(MoveTemp(command));
    if (!gestureOpen) {
        OnEditCommitted(false);
    }
}

void TilemapEditSession::PushUndoCommand(TilemapEditCommand&& command) noexcept {
    undoStack.PushBack(MoveTemp(command));
    redoStack.Clear();
    while (undoStack.GetSize() > maxUndoSteps) {
        undoStack.RemoveAt(0U);
    }
}

void TilemapEditSession::BumpRevisionForEdits(
        const Array<TilemapCellEdit>& edits,
        const TilemapEditChangeKind changeKind) noexcept {
    if (edits.IsEmpty()) {
        return;
    }
    TilemapCellRegion region = TilemapCellRegion::SingleCell(edits[0].layerIndex, edits[0].cellX, edits[0].cellY);
    for (std::size_t i = 1; i < edits.GetSize(); ++i) {
        if (edits[i].layerIndex == region.layerIndex) {
            region.ExpandToInclude(edits[i].cellX, edits[i].cellY);
        }
    }
    static_cast<void>(revisionTracker.RecordRegionChange(region.layerIndex, region, changeKind));
}

void TilemapEditSession::ApplyCellEdits(const Array<TilemapCellEdit>& edits, const bool useAfter) noexcept {
    if (tilemap == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < edits.GetSize(); ++i) {
        const TilemapCellEdit& edit = edits[i];
        const TileCell& value = useAfter ? edit.after : edit.before;
        DocumentCell(edit.layerIndex, edit.cellX, edit.cellY) = value;
        tilemap->SetTileCell(edit.layerIndex, edit.cellX, edit.cellY, value);
    }
}

void TilemapEditSession::SyncFullDocumentToRuntime() noexcept {
    if (tilemap == nullptr) {
        return;
    }
    while (tilemap->GetLayerCount() > document.tileLayers.GetSize() && tilemap->GetLayerCount() > 1U) {
        tilemap->RemoveLayer(tilemap->GetLayerCount() - 1U);
    }
    while (tilemap->GetLayerCount() < document.tileLayers.GetSize()) {
        static_cast<void>(tilemap->AddLayer("Layer"));
    }
    tilemap->Resize(document.mapWidth, document.mapHeight);
    for (std::size_t li = 0; li < document.tileLayers.GetSize(); ++li) {
        TilemapLayer& runtimeLayer = tilemap->GetLayer(static_cast<std::uint32_t>(li));
        const TilemapDocumentTileLayer& src = document.tileLayers[li];
        runtimeLayer.name = src.name;
        runtimeLayer.visible = src.visible;
        runtimeLayer.orderInLayerOffset = src.orderInLayerOffset;
        runtimeLayer.contributeCollision = src.contributeCollision;
        runtimeLayer.contributeGameplayGrid = src.contributeGameplayGrid;
        runtimeLayer.sortMode = src.sortMode;
        runtimeLayer.cells = src.cells;
    }
}

void TilemapEditSession::BeginGesture() noexcept {
    if (gestureOpen) {
        EndGesture();
    }
    gestureOpen = true;
    pendingEdits.Clear();
}

void TilemapEditSession::EndGesture() noexcept {
    if (!gestureOpen) {
        return;
    }
    gestureOpen = false;
    CommitPendingEdits(TilemapEditChangeKind::CellPaint);
    OnEditCommitted(true);
}

void TilemapEditSession::TryAutoRebake() noexcept {
    if (!IsAttached() || autoRebakePolicy == TilemapEditAutoRebakePolicy::Off || owner == nullptr) {
        return;
    }
    static_cast<void>(derivedRebaker_.RebakeForRevision(
            *owner, revisionTracker.GetLastRevision(), derivedRebakeOptions, rebakeWorld, rebakePhysics));
}

void TilemapEditSession::RunValidationIfEnabled() noexcept {
    if (!validateAfterCommit || !IsAttached()) {
        return;
    }
    TilemapEditValidator::Options options{};
    if (tilemap != nullptr) {
        if (const SharedPtr<Tileset>& tileset = tilemap->GetTileset(); tileset) {
            options.runtimeMaxTileIdExclusive = tileset->GetCellCount();
        }
    }
    const TilemapEditValidationReport report = validator_.Validate(document, options);
    lastValidationSummary = report.FormatSummary();
}

void TilemapEditSession::OnEditCommitted(const bool fromGestureEnd) noexcept {
    RunValidationIfEnabled();
    if (autoRebakePolicy == TilemapEditAutoRebakePolicy::AfterEachCommit && !gestureOpen) {
        TryAutoRebake();
    } else if (autoRebakePolicy == TilemapEditAutoRebakePolicy::AfterGestureOnly && fromGestureEnd) {
        TryAutoRebake();
    }
}

bool TilemapEditSession::ApplyBrushAt(
        const std::uint32_t originCellX,
        const std::uint32_t originCellY,
        const std::uint32_t layerIndex,
        const TilemapBrush& useBrush) noexcept {
    if (!IsAttached() || layerIndex >= document.tileLayers.GetSize()) {
        return false;
    }

    auto applyAt = [&](const std::uint32_t cellX, const std::uint32_t cellY, const TileCell& after) -> bool {
        if (!InBounds(cellX, cellY)) {
            return false;
        }
        const TileCell before = DocumentCellConst(layerIndex, cellX, cellY);
        if (CellsEqual(before, after)) {
            return false;
        }
        DocumentCell(layerIndex, cellX, cellY) = after;
        tilemap->SetTileCell(layerIndex, cellX, cellY, after);
        RecordCellMutation(layerIndex, cellX, cellY, before, after);
        return true;
    };

    if (useBrush.mode == TilemapBrush::Mode::Stamp) {
        bool any = false;
        for (std::uint32_t sy = 0; sy < useBrush.stamp.height; ++sy) {
            for (std::uint32_t sx = 0; sx < useBrush.stamp.width; ++sx) {
                const std::uint32_t cellX = originCellX + sx + static_cast<std::uint32_t>(useBrush.stamp.anchorCellX);
                const std::uint32_t cellY = originCellY + sy + static_cast<std::uint32_t>(useBrush.stamp.anchorCellY);
                const TileCell after =
                        useBrush.ResolveCell(originCellX, originCellY, cellX, cellY);
                if (!after.IsEmpty()) {
                    any = applyAt(cellX, cellY, after) || any;
                }
            }
        }
        return any;
    }

    const TileCell after = useBrush.ResolveCell(originCellX, originCellY, originCellX, originCellY);
    return applyAt(originCellX, originCellY, after);
}

bool TilemapEditSession::PaintCell(const std::uint32_t cellX, const std::uint32_t cellY) noexcept {
    return ApplyBrushAt(cellX, cellY, activeLayerIndex, brush);
}

bool TilemapEditSession::EraseCell(const std::uint32_t cellX, const std::uint32_t cellY) noexcept {
    TilemapBrush eraseBrush{};
    eraseBrush.mode = TilemapBrush::Mode::Single;
    eraseBrush.single = TileCell::Empty();
    return ApplyBrushAt(cellX, cellY, activeLayerIndex, eraseBrush);
}

bool TilemapEditSession::PaintLine(
        const std::uint32_t x0,
        const std::uint32_t y0,
        const std::uint32_t x1,
        const std::uint32_t y1) noexcept {
    if (!IsAttached()) {
        return false;
    }
    Array<std::pair<std::uint32_t, std::uint32_t>> cells{};
    CollectLineCells(static_cast<std::int32_t>(x0), static_cast<std::int32_t>(y0), static_cast<std::int32_t>(x1),
            static_cast<std::int32_t>(y1), cells);
    bool any = false;
    const bool autoGesture = !gestureOpen;
    if (autoGesture) {
        BeginGesture();
    }
    for (std::size_t i = 0; i < cells.GetSize(); ++i) {
        if (InBounds(cells[i].first, cells[i].second)) {
            any = PaintCell(cells[i].first, cells[i].second) || any;
        }
    }
    if (autoGesture) {
        EndGesture();
    }
    return any;
}

bool TilemapEditSession::PaintRect(
        const std::uint32_t x0,
        const std::uint32_t y0,
        const std::uint32_t x1,
        const std::uint32_t y1,
        const bool filled) noexcept {
    if (!IsAttached()) {
        return false;
    }
    const std::uint32_t minX = std::min(x0, x1);
    const std::uint32_t maxX = std::max(x0, x1);
    const std::uint32_t minY = std::min(y0, y1);
    const std::uint32_t maxY = std::max(y0, y1);

    const bool autoGesture = !gestureOpen;
    if (autoGesture) {
        BeginGesture();
    }
    bool any = false;
    if (filled) {
        for (std::uint32_t y = minY; y <= maxY; ++y) {
            for (std::uint32_t x = minX; x <= maxX; ++x) {
                if (InBounds(x, y)) {
                    any = PaintCell(x, y) || any;
                }
            }
        }
    } else {
        for (std::uint32_t x = minX; x <= maxX; ++x) {
            if (InBounds(x, minY)) {
                any = PaintCell(x, minY) || any;
            }
            if (maxY != minY && InBounds(x, maxY)) {
                any = PaintCell(x, maxY) || any;
            }
        }
        for (std::uint32_t y = minY + 1U; y < maxY; ++y) {
            if (InBounds(minX, y)) {
                any = PaintCell(minX, y) || any;
            }
            if (maxX != minX && InBounds(maxX, y)) {
                any = PaintCell(maxX, y) || any;
            }
        }
    }
    if (autoGesture) {
        EndGesture();
    }
    return any;
}

bool TilemapEditSession::FloodFill(const std::uint32_t cellX, const std::uint32_t cellY) noexcept {
    if (!IsAttached() || !InBounds(cellX, cellY) || activeLayerIndex >= document.tileLayers.GetSize()) {
        return false;
    }
    const TileCell match = DocumentCellConst(activeLayerIndex, cellX, cellY);
    const TileCell fillCell = brush.ResolveCell(cellX, cellY, cellX, cellY);
    if (CellsEqual(match, fillCell)) {
        return false;
    }

    Array<std::pair<std::uint32_t, std::uint32_t>> stack{};
    stack.PushBack({cellX, cellY});
    bool any = false;
    const bool autoGesture = !gestureOpen;
    if (autoGesture) {
        BeginGesture();
    }

    while (!stack.IsEmpty()) {
        const std::pair<std::uint32_t, std::uint32_t> current = stack.GetLast();
        stack.PopBack();
        const std::uint32_t x = current.first;
        const std::uint32_t y = current.second;
        if (!InBounds(x, y)) {
            continue;
        }
        if (!CellsEqual(DocumentCellConst(activeLayerIndex, x, y), match)) {
            continue;
        }
        const TileCell after = brush.ResolveCell(cellX, cellY, x, y);
        const TileCell before = DocumentCellConst(activeLayerIndex, x, y);
        if (CellsEqual(before, after)) {
            continue;
        }
        DocumentCell(activeLayerIndex, x, y) = after;
        tilemap->SetTileCell(activeLayerIndex, x, y, after);
        RecordCellMutation(activeLayerIndex, x, y, before, after);
        any = true;
        if (x > 0U) {
            stack.PushBack({x - 1U, y});
        }
        if (x + 1U < document.mapWidth) {
            stack.PushBack({x + 1U, y});
        }
        if (y > 0U) {
            stack.PushBack({x, y - 1U});
        }
        if (y + 1U < document.mapHeight) {
            stack.PushBack({x, y + 1U});
        }
    }

    if (autoGesture) {
        EndGesture();
    }
    return any;
}

bool TilemapEditSession::ResizeMap(const std::uint32_t newWidth, const std::uint32_t newHeight) noexcept {
    if (!IsAttached() || newWidth == 0U || newHeight == 0U) {
        return false;
    }
    if (document.mapWidth == newWidth && document.mapHeight == newHeight) {
        return false;
    }
    if (gestureOpen) {
        EndGesture();
    }

    TilemapEditCommand command{};
    command.kind = TilemapEditCommandKind::MapResize;
    command.changeKind = TilemapEditChangeKind::MapResize;
    command.documentBeforeResize = document;

    ResizeDocumentLayers(document, newWidth, newHeight);
    SyncFullDocumentToRuntime();
    command.documentAfterResize = document;

    static_cast<void>(revisionTracker.RecordFullMapChange(TilemapEditChangeKind::MapResize));
    PushUndoCommand(MoveTemp(command));
    OnEditCommitted(false);
    return true;
}

TilemapDerivedDataRebaker::Result TilemapEditSession::RebakeDerivedDataForLastEdit(GameObject& owner) const noexcept {
    return RebakeDerivedDataForLastEdit(owner, TilemapDerivedDataRebaker::Options{}, nullptr, nullptr);
}

TilemapDerivedDataRebaker::Result TilemapEditSession::RebakeDerivedDataForLastEdit(
        GameObject& owner,
        const TilemapDerivedDataRebaker::Options& options,
        GameWorld* worldForPhysics,
        PhysicsSubsystem* physics) const noexcept {
    return derivedRebaker_.RebakeForRevision(
            owner, revisionTracker.GetLastRevision(), options, worldForPhysics, physics);
}

void TilemapEditSession::ClearHistory() noexcept {
    undoStack.Clear();
    redoStack.Clear();
    pendingEdits.Clear();
    gestureOpen = false;
}

bool TilemapEditSession::Undo() noexcept {
    if (!IsAttached() || undoStack.IsEmpty()) {
        return false;
    }
    if (gestureOpen) {
        EndGesture();
    }

    TilemapEditCommand command = MoveTemp(undoStack.GetLast());
    undoStack.PopBack();

    if (command.kind == TilemapEditCommandKind::CellEdits) {
        ApplyCellEdits(command.cellEdits, false);
        TilemapCellRegion region{};
        region.layerIndex = TilemapCellRegion::kInvalidLayer;
        if (!command.cellEdits.IsEmpty()) {
            region = TilemapCellRegion::SingleCell(
                    command.cellEdits[0].layerIndex, command.cellEdits[0].cellX, command.cellEdits[0].cellY);
            for (std::size_t i = 1; i < command.cellEdits.GetSize(); ++i) {
                if (command.cellEdits[i].layerIndex == region.layerIndex) {
                    region.ExpandToInclude(command.cellEdits[i].cellX, command.cellEdits[i].cellY);
                }
            }
        }
        static_cast<void>(revisionTracker.RecordRegionChange(region.layerIndex, region, command.changeKind));
    } else {
        document = command.documentBeforeResize;
        SyncFullDocumentToRuntime();
        static_cast<void>(revisionTracker.RecordFullMapChange(TilemapEditChangeKind::MapResize));
    }

    redoStack.PushBack(MoveTemp(command));
    return true;
}

bool TilemapEditSession::Redo() noexcept {
    if (!IsAttached() || redoStack.IsEmpty()) {
        return false;
    }
    if (gestureOpen) {
        EndGesture();
    }

    TilemapEditCommand command = MoveTemp(redoStack.GetLast());
    redoStack.PopBack();

    if (command.kind == TilemapEditCommandKind::CellEdits) {
        ApplyCellEdits(command.cellEdits, true);
        TilemapCellRegion region{};
        region.layerIndex = TilemapCellRegion::kInvalidLayer;
        if (!command.cellEdits.IsEmpty()) {
            region = TilemapCellRegion::SingleCell(
                    command.cellEdits[0].layerIndex, command.cellEdits[0].cellX, command.cellEdits[0].cellY);
            for (std::size_t i = 1; i < command.cellEdits.GetSize(); ++i) {
                if (command.cellEdits[i].layerIndex == region.layerIndex) {
                    region.ExpandToInclude(command.cellEdits[i].cellX, command.cellEdits[i].cellY);
                }
            }
        }
        static_cast<void>(revisionTracker.RecordRegionChange(region.layerIndex, region, command.changeKind));
    } else {
        document = command.documentAfterResize;
        SyncFullDocumentToRuntime();
        static_cast<void>(revisionTracker.RecordFullMapChange(TilemapEditChangeKind::MapResize));
    }

    undoStack.PushBack(MoveTemp(command));
    return true;
}

}  // namespace Spark
