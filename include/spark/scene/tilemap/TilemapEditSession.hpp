#pragma once

#include "spark/core/Array.hpp"
#include "spark/scene/tilemap/TilemapBrush.hpp"
#include "spark/scene/tilemap/TilemapDocument.hpp"
#include "spark/scene/tilemap/TilemapDocumentCapture.hpp"
#include "spark/scene/tilemap/TilemapDerivedDataRebake.hpp"
#include "spark/scene/tilemap/TilemapEditRevision.hpp"
#include "spark/scene/tilemap/TilemapEditValidator.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class GameWorld;
class PhysicsSubsystem;
class TilemapComponent;

struct TilemapCellEdit {
    std::uint32_t layerIndex = 0U;
    std::uint32_t cellX = 0U;
    std::uint32_t cellY = 0U;
    TileCell before{};
    TileCell after{};
};

enum class TilemapEditCommandKind : std::uint8_t {
    CellEdits = 0,
    MapResize = 1,
};

struct TilemapEditCommand {
    TilemapEditCommandKind kind = TilemapEditCommandKind::CellEdits;
    TilemapEditChangeKind changeKind = TilemapEditChangeKind::CellPaint;
    Array<TilemapCellEdit> cellEdits{};
    TilemapDocument documentBeforeResize{};
    TilemapDocument documentAfterResize{};
};

enum class TilemapEditAutoRebakePolicy : std::uint8_t {
    Off = 0,
    AfterEachCommit = 1,
    AfterGestureOnly = 2,
};

struct TilemapEditSessionOptions {
    std::uint32_t activeLayerIndex = 0U;
    std::size_t maxUndoSteps = 64U;
    TilemapEditAutoRebakePolicy autoRebakePolicy = TilemapEditAutoRebakePolicy::Off;
    bool validateAfterCommit = false;
    TilemapDerivedDataRebaker::Options derivedRebakeOptions{};
};

/**
 * In-memory <c>TilemapDocument</c> editor with undo/redo, brush tools, and revision tracking.
 * Mutations sync to the owner's <c>TilemapComponent</c>.
 */
class TilemapEditSession final {
public:
    [[nodiscard]] bool IsAttached() const noexcept { return owner != nullptr && tilemap != nullptr; }

    /** Captures the current map from <c>owner</c> and prepares undo stacks. */
    [[nodiscard]] bool Attach(GameObject& owner) noexcept;
    [[nodiscard]] bool Attach(GameObject& owner, const TilemapEditSessionOptions& options) noexcept;

    void Detach() noexcept;

    void SetDerivedRebakeContext(GameWorld* worldForPhysics, PhysicsSubsystem* physics = nullptr) noexcept;
    [[nodiscard]] const Utf8String& GetLastValidationSummary() const noexcept { return lastValidationSummary; }

    [[nodiscard]] const TilemapDocument& GetDocument() const noexcept { return document; }
    [[nodiscard]] TilemapDocument& GetDocumentMutable() noexcept { return document; }

    [[nodiscard]] std::uint32_t GetActiveLayerIndex() const noexcept { return activeLayerIndex; }
    void SetActiveLayerIndex(const std::uint32_t layerIndex) noexcept { activeLayerIndex = layerIndex; }

    [[nodiscard]] const TilemapBrush& GetBrush() const noexcept { return brush; }
    void SetBrush(const TilemapBrush& inBrush) noexcept { brush = inBrush; }

    [[nodiscard]] const TilemapRevisionTracker& GetRevisionTracker() const noexcept { return revisionTracker; }
    [[nodiscard]] const TilemapEditRevision& GetLastRevision() const noexcept {
        return revisionTracker.GetLastRevision();
    }

    /** Coalesce multiple paint calls (e.g. mouse drag) into one undo step. */
    void BeginGesture() noexcept;
    void EndGesture() noexcept;
    [[nodiscard]] bool IsGestureOpen() const noexcept { return gestureOpen; }

    [[nodiscard]] bool PaintCell(const std::uint32_t cellX, const std::uint32_t cellY) noexcept;
    [[nodiscard]] bool EraseCell(const std::uint32_t cellX, const std::uint32_t cellY) noexcept;
    [[nodiscard]] bool PaintLine(
            const std::uint32_t x0,
            const std::uint32_t y0,
            const std::uint32_t x1,
            const std::uint32_t y1) noexcept;
    [[nodiscard]] bool PaintRect(
            const std::uint32_t x0,
            const std::uint32_t y0,
            const std::uint32_t x1,
            const std::uint32_t y1,
            const bool filled) noexcept;
    [[nodiscard]] bool FloodFill(const std::uint32_t cellX, const std::uint32_t cellY) noexcept;
    [[nodiscard]] bool ResizeMap(const std::uint32_t newWidth, const std::uint32_t newHeight) noexcept;

    [[nodiscard]] bool CanUndo() const noexcept { return !undoStack.IsEmpty(); }
    [[nodiscard]] bool CanRedo() const noexcept { return !redoStack.IsEmpty(); }
    [[nodiscard]] bool Undo() noexcept;
    [[nodiscard]] bool Redo() noexcept;

    void ClearHistory() noexcept;

    /** Rebakes gameplay grid / autotile (and optional query statics) for <c>GetLastRevision()</c>. */
    [[nodiscard]] TilemapDerivedDataRebaker::Result RebakeDerivedDataForLastEdit(GameObject& owner) const noexcept;

    [[nodiscard]] TilemapDerivedDataRebaker::Result RebakeDerivedDataForLastEdit(
            GameObject& owner,
            const TilemapDerivedDataRebaker::Options& options,
            GameWorld* worldForPhysics = nullptr,
            PhysicsSubsystem* physics = nullptr) const noexcept;

private:
    [[nodiscard]] bool InBounds(const std::uint32_t cellX, const std::uint32_t cellY) const noexcept;
    [[nodiscard]] TileCell& DocumentCell(
            const std::uint32_t layerIndex,
            const std::uint32_t cellX,
            const std::uint32_t cellY) noexcept;
    [[nodiscard]] TileCell DocumentCellConst(
            const std::uint32_t layerIndex,
            const std::uint32_t cellX,
            const std::uint32_t cellY) const noexcept;

    [[nodiscard]] bool ApplyBrushAt(
            const std::uint32_t originCellX,
            const std::uint32_t originCellY,
            const std::uint32_t layerIndex,
            const TilemapBrush& useBrush) noexcept;

    void RecordCellMutation(
            const std::uint32_t layerIndex,
            const std::uint32_t cellX,
            const std::uint32_t cellY,
            const TileCell& before,
            const TileCell& after) noexcept;

    void CommitPendingEdits(const TilemapEditChangeKind changeKind) noexcept;
    void PushUndoCommand(TilemapEditCommand&& command) noexcept;
    void ApplyCellEdits(const Array<TilemapCellEdit>& edits, const bool useAfter) noexcept;
    void SyncFullDocumentToRuntime() noexcept;
    void BumpRevisionForEdits(
            const Array<TilemapCellEdit>& edits,
            const TilemapEditChangeKind changeKind) noexcept;

    void OnEditCommitted(const bool fromGestureEnd) noexcept;
    void TryAutoRebake() noexcept;
    void RunValidationIfEnabled() noexcept;

    GameObject* owner = nullptr;
    TilemapComponent* tilemap = nullptr;
    TilemapDocument document{};
    TilemapBrush brush{};
    TilemapRevisionTracker revisionTracker{};
    std::uint32_t activeLayerIndex = 0U;
    std::size_t maxUndoSteps = 64U;
    TilemapEditAutoRebakePolicy autoRebakePolicy = TilemapEditAutoRebakePolicy::Off;
    bool validateAfterCommit = false;
    TilemapDerivedDataRebaker::Options derivedRebakeOptions{};
    GameWorld* rebakeWorld = nullptr;
    PhysicsSubsystem* rebakePhysics = nullptr;
    Utf8String lastValidationSummary{};

    bool gestureOpen = false;
    Array<TilemapCellEdit> pendingEdits{};
    Array<TilemapEditCommand> undoStack{};
    Array<TilemapEditCommand> redoStack{};

    TilemapDocumentCapturer documentCapturer_{};
    TilemapDerivedDataRebaker derivedRebaker_{};
    TilemapEditValidator validator_{};
};

}  // namespace Spark
