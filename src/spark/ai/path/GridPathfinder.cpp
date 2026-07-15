#include "spark/ai/path/GridPathfinder.hpp"

#include <cmath>

namespace Spark {

void GridBitmapWalkability::Resize(const std::int32_t w, const std::int32_t h) {
    width = w;
    height = h;
    cells.Clear();
    if (w > 0 && h > 0) {
        const std::size_t n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        cells.Reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            cells.PushBack(0);
        }
    }
}

std::size_t GridBitmapWalkability::Index_(const std::int32_t x, const std::int32_t y) const noexcept {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

void GridBitmapWalkability::SetBlocked(const std::int32_t x, const std::int32_t y, const bool blocked) noexcept {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }
    cells[Index_(x, y)] = blocked ? 1 : 0;
}

bool GridBitmapWalkability::IsWalkable(const std::int32_t x, const std::int32_t y) const noexcept {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return false;
    }
    return cells[Index_(x, y)] == 0;
}

namespace {

struct ClosedNode {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t parentX = -1;
    std::int32_t parentY = -1;
};

struct OpenRich {
    std::int32_t x = 0;
    std::int32_t y = 0;
    float g = 0.0F;
    float f = 0.0F;
    std::int32_t parentX = -1;
    std::int32_t parentY = -1;
};

[[nodiscard]] float Heuristic(const std::int32_t ax, const std::int32_t ay, const std::int32_t bx, const std::int32_t by) noexcept {
    const float dx = static_cast<float>(bx - ax);
    const float dy = static_cast<float>(by - ay);
    return std::fabs(dx) + std::fabs(dy);
}

[[nodiscard]] const ClosedNode* FindClosed(const Array<ClosedNode>& closed, const std::int32_t x, const std::int32_t y) noexcept {
    for (std::size_t i = 0; i < closed.GetSize(); ++i) {
        if (closed[i].x == x && closed[i].y == y) {
            return &closed[i];
        }
    }
    return nullptr;
}

}  // namespace

bool GridPathfinder::FindPath4(
        const IGridWalkability& grid,
        const Cell& start,
        const Cell& goal,
        Array<Cell>& outCells) {
    outCells.Clear();
    if (!grid.IsWalkable(start.x, start.y) || !grid.IsWalkable(goal.x, goal.y)) {
        return false;
    }
    if (start.x == goal.x && start.y == goal.y) {
        outCells.PushBack(start);
        return true;
    }

    Array<OpenRich> openR;
    Array<ClosedNode> closed;

    OpenRich s0{};
    s0.x = start.x;
    s0.y = start.y;
    s0.g = 0.0F;
    s0.f = Heuristic(start.x, start.y, goal.x, goal.y);
    s0.parentX = -1;
    s0.parentY = -1;
    openR.PushBack(s0);

    auto findOpenRichMin = [](const Array<OpenRich>& o, std::size_t& outI) noexcept {
        outI = 0;
        float bf = o[0].f;
        for (std::size_t i = 1; i < o.GetSize(); ++i) {
            if (o[i].f < bf) {
                bf = o[i].f;
                outI = i;
            }
        }
    };

    auto findOpenRich = [](Array<OpenRich>& o, const std::int32_t x, const std::int32_t y) noexcept -> OpenRich* {
        for (std::size_t i = 0; i < o.GetSize(); ++i) {
            if (o[i].x == x && o[i].y == y) {
                return &o[i];
            }
        }
        return nullptr;
    };

    auto inClosed = [](const Array<ClosedNode>& cl, const std::int32_t x, const std::int32_t y) noexcept {
        for (std::size_t i = 0; i < cl.GetSize(); ++i) {
            if (cl[i].x == x && cl[i].y == y) {
                return true;
            }
        }
        return false;
    };

    constexpr std::int32_t kDx[4] = {1, -1, 0, 0};
    constexpr std::int32_t kDy[4] = {0, 0, 1, -1};

    while (!openR.IsEmpty()) {
        std::size_t mi = 0;
        findOpenRichMin(openR, mi);
        const OpenRich cur = openR[mi];
        openR.RemoveAt(mi);

        ClosedNode done{};
        done.x = cur.x;
        done.y = cur.y;
        done.parentX = cur.parentX;
        done.parentY = cur.parentY;
        closed.PushBack(done);

        if (cur.x == goal.x && cur.y == goal.y) {
            Array<Cell> rev;
            std::int32_t wx = goal.x;
            std::int32_t wy = goal.y;
            rev.PushBack(Cell{wx, wy});
            for (;;) {
                const ClosedNode* node = FindClosed(closed, wx, wy);
                if (node == nullptr || node->parentX < 0) {
                    break;
                }
                wx = node->parentX;
                wy = node->parentY;
                rev.PushBack(Cell{wx, wy});
                if (wx == start.x && wy == start.y) {
                    break;
                }
            }
            for (std::size_t i = rev.GetSize(); i > 0; --i) {
                outCells.PushBack(rev[i - 1]);
            }
            return true;
        }

        for (int k = 0; k < 4; ++k) {
            const std::int32_t nx = cur.x + kDx[k];
            const std::int32_t ny = cur.y + kDy[k];
            if (!grid.IsWalkable(nx, ny)) {
                continue;
            }
            if (inClosed(closed, nx, ny)) {
                continue;
            }
            const float tentativeG = cur.g + 1.0F;
            OpenRich* ex = findOpenRich(openR, nx, ny);
            if (ex != nullptr) {
                if (tentativeG < ex->g) {
                    ex->g = tentativeG;
                    ex->f = tentativeG + Heuristic(nx, ny, goal.x, goal.y);
                    ex->parentX = cur.x;
                    ex->parentY = cur.y;
                }
            } else {
                OpenRich n{};
                n.x = nx;
                n.y = ny;
                n.g = tentativeG;
                n.f = tentativeG + Heuristic(nx, ny, goal.x, goal.y);
                n.parentX = cur.x;
                n.parentY = cur.y;
                openR.PushBack(n);
            }
        }
    }

    return false;
}

void GridPathfinder::CellsToWorldPolyline(
        const Array<Cell>& cells,
        const Vector2& gridOriginXZ,
        const float cellSize,
        Array<Vector2>& outWorldXZ) {
    outWorldXZ.Clear();
    const float cs = cellSize > 1.0e-4F ? cellSize : 1.0e-4F;
    for (std::size_t i = 0; i < cells.GetSize(); ++i) {
        const Cell& c = cells[i];
        const float wx = gridOriginXZ.x + (static_cast<float>(c.x) + 0.5F) * cs;
        const float wz = gridOriginXZ.y + (static_cast<float>(c.y) + 0.5F) * cs;
        outWorldXZ.PushBack(Vector2{wx, wz});
    }
}

}  // namespace Spark
