#include "spark/scene/tilemap/TilemapGameplayPlacement.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/camera/Camera2D.hpp"
#include "spark/scene/tilemap/TilemapLayer.hpp"

#include <cmath>
#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] bool ScreenToWorldRay(
        const int fbW,
        const int fbH,
        const float px,
        const float py,
        const Matrix4& invViewProj,
        Vector3& outOrigin,
        Vector3& outDir) noexcept {
    if (fbW <= 0 || fbH <= 0) {
        return false;
    }
    const float ndcX = (2.0F * px / static_cast<float>(fbW)) - 1.0F;
    const float ndcY = (2.0F * py / static_cast<float>(fbH)) - 1.0F;
    const Vector4 p0 = invViewProj * Vector4(ndcX, ndcY, 0.0F, 1.0F);
    const Vector4 p1 = invViewProj * Vector4(ndcX, ndcY, 1.0F, 1.0F);
    if (std::fabs(p0.w) < 1.0e-6F || std::fabs(p1.w) < 1.0e-6F) {
        return false;
    }
    const Vector3 o = (p0 * (1.0F / p0.w)).ToVector3();
    Vector3 d = (p1 * (1.0F / p1.w)).ToVector3() - o;
    if (d.LengthSquared() < 1.0e-12F) {
        return false;
    }
    d = d.Normalized();
    outOrigin = o;
    outDir = d;
    return true;
}

[[nodiscard]] float CellDistanceSqToWorld(
        const TilemapGridFrame& frame,
        const GridPathfinder::Cell& cell,
        const Vector2& worldXY) noexcept {
    const Vector2 center = frame.CellCenterToWorldXY(cell);
    const float dx = center.x - worldXY.x;
    const float dy = center.y - worldXY.y;
    return dx * dx + dy * dy;
}

[[nodiscard]] std::size_t VisitIndex(const std::int32_t width, const std::int32_t x, const std::int32_t y) noexcept {
    return static_cast<std::size_t>(y * width + x);
}

}  // namespace

void ApplyDefaultGameplayLayerFlags(TilemapComponent& tilemap) noexcept {
    for (std::uint32_t layerIndex = 0U; layerIndex < tilemap.GetLayerCount(); ++layerIndex) {
        TilemapLayer& layer = tilemap.GetLayer(layerIndex);
        const char* name = layer.name.CStr();
        if (name == nullptr) {
            continue;
        }
        if (std::strcmp(name, "Carts") == 0) {
            layer.contributeCollision = false;
            layer.contributeGameplayGrid = false;
        } else if (std::strcmp(name, "Objects") == 0) {
            layer.contributeGameplayGrid = false;
        }
    }
}

bool IsWalkableMapCell(
        const IGridWalkability& walk,
        const TilemapGridFrame& frame,
        const std::int32_t x,
        const std::int32_t y) noexcept {
    const GridPathfinder::Cell cell{x, y};
    if (!frame.IsCellInBounds(cell)) {
        return false;
    }
    return walk.IsWalkable(x, y);
}

void CollectReachableWalkableCells(
        const IGridWalkability& walk,
        const GridPathfinder::Cell& start,
        Array<GridPathfinder::Cell>& out) noexcept {
    out.Clear();
    if (!walk.IsWalkable(start.x, start.y)) {
        return;
    }
    const std::int32_t width = walk.Width();
    const std::int32_t height = walk.Height();
    if (width <= 0 || height <= 0) {
        return;
    }

    Array<std::uint8_t> visited{};
    visited.Resize(static_cast<std::size_t>(width * height));
    for (std::size_t i = 0; i < visited.GetSize(); ++i) {
        visited[i] = 0U;
    }

    Array<GridPathfinder::Cell> queue{};
    queue.PushBack(start);
    visited[VisitIndex(width, start.x, start.y)] = 1U;
    out.PushBack(start);

    std::size_t head = 0U;
    while (head < queue.GetSize()) {
        const GridPathfinder::Cell current = queue[head++];
        const std::int32_t neighbors[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (const auto& offset : neighbors) {
            const GridPathfinder::Cell next{current.x + offset[0], current.y + offset[1]};
            if (!walk.IsWalkable(next.x, next.y)) {
                continue;
            }
            const std::size_t index = VisitIndex(width, next.x, next.y);
            if (visited[index] != 0U) {
                continue;
            }
            visited[index] = 1U;
            queue.PushBack(next);
            out.PushBack(next);
        }
    }
}

bool PickSpawnInLargestWalkableRegion(
        const IGridWalkability& walk,
        const TilemapGridFrame& frame,
        const Vector2& hintWorldXY,
        const std::size_t minReachableCells,
        GridPathfinder::Cell& outCell) noexcept {
    const std::int32_t width = walk.Width();
    const std::int32_t height = walk.Height();
    if (width <= 0 || height <= 0) {
        return false;
    }

    const std::size_t cellCount = static_cast<std::size_t>(width * height);
    Array<std::uint8_t> visited{};
    visited.Resize(cellCount);
    for (std::size_t i = 0; i < cellCount; ++i) {
        visited[i] = 0U;
    }

    GridPathfinder::Cell bestRegionSeed{};
    std::size_t bestRegionSize = 0U;

    for (std::int32_t y = 0; y < height; ++y) {
        for (std::int32_t x = 0; x < width; ++x) {
            const std::size_t startIndex = VisitIndex(width, x, y);
            if (visited[startIndex] != 0U || !walk.IsWalkable(x, y)) {
                continue;
            }
            Array<GridPathfinder::Cell> region{};
            CollectReachableWalkableCells(walk, {x, y}, region);
            for (std::size_t ri = 0; ri < region.GetSize(); ++ri) {
                const GridPathfinder::Cell& c = region[ri];
                visited[VisitIndex(width, c.x, c.y)] = 1U;
            }
            if (region.GetSize() > bestRegionSize) {
                bestRegionSize = region.GetSize();
                bestRegionSeed = {x, y};
            }
        }
    }

    if (bestRegionSize < minReachableCells) {
        return false;
    }

    Array<GridPathfinder::Cell> primary{};
    CollectReachableWalkableCells(walk, bestRegionSeed, primary);
    if (primary.GetSize() < minReachableCells) {
        return false;
    }

    float bestDistSq = 0.0F;
    bool found = false;
    for (std::size_t i = 0; i < primary.GetSize(); ++i) {
        const float distSq = CellDistanceSqToWorld(frame, primary[i], hintWorldXY);
        if (!found || distSq < bestDistSq) {
            bestDistSq = distSq;
            outCell = primary[i];
            found = true;
        }
    }
    return found;
}

bool TryPickWalkableGridCellFromScreen(
        const Camera2D& camera,
        const TilemapGridFrame& frame,
        const IGridWalkability& walk,
        const float framebufferWidth,
        const float framebufferHeight,
        const float cursorX,
        const float cursorY,
        GridPathfinder::Cell& outCell) noexcept {
    if (framebufferWidth < 1.0F || framebufferHeight < 1.0F) {
        return false;
    }
    const Matrix4 viewProj = camera.ViewProjection(framebufferWidth, framebufferHeight);
    Matrix4 invVp{};
    if (!viewProj.TryInvert(invVp)) {
        return false;
    }
    Vector3 rayOrigin{};
    Vector3 rayDir{};
    if (!ScreenToWorldRay(
                static_cast<int>(framebufferWidth),
                static_cast<int>(framebufferHeight),
                cursorX,
                cursorY,
                invVp,
                rayOrigin,
                rayDir)) {
        return false;
    }
    if (std::fabs(rayDir.z) < 1.0e-5F) {
        return false;
    }
    const float t = -rayOrigin.z / rayDir.z;
    const Vector2 world{rayOrigin.x + rayDir.x * t, rayOrigin.y + rayDir.y * t};
    outCell = frame.WorldXYToCell(world);
    return IsWalkableMapCell(walk, frame, outCell.x, outCell.y);
}

void SortCellsByDistanceFromWorld(
        Array<GridPathfinder::Cell>& cells,
        const TilemapGridFrame& frame,
        const Vector2& worldXY) noexcept {
    for (std::size_t i = 0; i + 1U < cells.GetSize(); ++i) {
        for (std::size_t j = i + 1U; j < cells.GetSize(); ++j) {
            const float di = CellDistanceSqToWorld(frame, cells[i], worldXY);
            const float dj = CellDistanceSqToWorld(frame, cells[j], worldXY);
            if (dj < di) {
                const GridPathfinder::Cell tmp = cells[i];
                cells[i] = cells[j];
                cells[j] = tmp;
            }
        }
    }
}

}  // namespace Spark
