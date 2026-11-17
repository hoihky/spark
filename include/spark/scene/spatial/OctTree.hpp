#pragma once

#include "spark/math/AxisAlignedBox.hpp"
#include "spark/math/Frustum.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Spark {

struct OctTreeOptions {
    int maxDepth = 14;
    int maxLeafSize = 8;
};

/**
 * Axis-aligned octree: each item is stored in the deepest fully containing child when possible;
 * otherwise it remains at an ancestor (multi-cell overlap).
 */
class OctTree final {
public:
    using Options = OctTreeOptions;

    void Clear() noexcept;
    void Reset(const AxisAlignedBox& rootBounds, Options options = OctTreeOptions());

    /** <c>itemBounds[itemIndex]</c> must be valid; array must cover all items inserted so far. */
    void Insert(int itemIndex, const AxisAlignedBox* itemBounds, std::size_t itemCount);

    template<typename Fn>
    void QueryFrustum(const Frustum& frustum, const AxisAlignedBox* itemBounds, Fn&& onItem) const {
        if (nodes.empty() || itemBounds == nullptr) {
            return;
        }
        QueryFrustumRecursive(0, frustum, itemBounds, onItem);
    }

    [[nodiscard]] bool IsEmpty() const noexcept { return nodes.empty(); }

private:
    struct Node {
        AxisAlignedBox cell{};
        std::vector<int> items;
        std::array<int, 8> children{};
        Node() { children.fill(-1); }
    };

    [[nodiscard]] static AxisAlignedBox OctantOf(const AxisAlignedBox& parent, int octantIndex) noexcept;
    [[nodiscard]] static bool CellFullyContains(const AxisAlignedBox& cell, const AxisAlignedBox& item) noexcept;

    void InsertRecursive(
            int nodeIndex,
            int itemIndex,
            const AxisAlignedBox* itemBounds,
            int depth);

    template<typename Fn>
    void QueryFrustumRecursive(int nodeIndex, const Frustum& frustum, const AxisAlignedBox* itemBounds, Fn&& onItem)
            const {
        if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes.size())) {
            return;
        }
        const Node& n = nodes[static_cast<std::size_t>(nodeIndex)];
        if (!frustum.Intersects(n.cell)) {
            return;
        }
        for (const int item : n.items) {
            const AxisAlignedBox& box = itemBounds[static_cast<std::size_t>(item)];
            if (frustum.Intersects(box)) {
                onItem(item);
            }
        }
        for (int o = 0; o < 8; ++o) {
            const int ci = n.children[static_cast<std::size_t>(o)];
            if (ci >= 0) {
                QueryFrustumRecursive(ci, frustum, itemBounds, onItem);
            }
        }
    }

    std::vector<Node> nodes{};
    Options treeOptions{};
};

}  // namespace Spark