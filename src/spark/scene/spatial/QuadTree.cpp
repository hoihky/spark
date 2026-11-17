#include "spark/scene/spatial/QuadTree.hpp"

#include "spark/math/AxisAlignedBox.hpp"

namespace Spark {

void QuadTree::Clear() noexcept {
    nodes.clear();
}

void QuadTree::Reset(const float minX, const float maxX, const float minZ, const float maxZ, Options options) {
    nodes.clear();
    treeOptions = options;
    Node root{};
    root.minX = minX;
    root.maxX = maxX;
    root.minZ = minZ;
    root.maxZ = maxZ;
    nodes.push_back(root);
}

bool QuadTree::QuadCellFullyContains(
        const float minX,
        const float maxX,
        const float minZ,
        const float maxZ,
        const AxisAlignedBox& item) noexcept {
    return item.min.x >= minX && item.max.x <= maxX && item.min.z >= minZ && item.max.z <= maxZ;
}

void QuadTree::ChildBounds(
        const float pminX,
        const float pmaxX,
        const float pminZ,
        const float pmaxZ,
        const int quadrant,
        float& cminX,
        float& cmaxX,
        float& cminZ,
        float& cmaxZ) noexcept {
    const float midX = (pminX + pmaxX) * 0.5F;
    const float midZ = (pminZ + pmaxZ) * 0.5F;
    switch (quadrant) {
        case 0:
            cminX = pminX;
            cmaxX = midX;
            cminZ = pminZ;
            cmaxZ = midZ;
            break;
        case 1:
            cminX = midX;
            cmaxX = pmaxX;
            cminZ = pminZ;
            cmaxZ = midZ;
            break;
        case 2:
            cminX = pminX;
            cmaxX = midX;
            cminZ = midZ;
            cmaxZ = pmaxZ;
            break;
        default:
            cminX = midX;
            cmaxX = pmaxX;
            cminZ = midZ;
            cmaxZ = pmaxZ;
            break;
    }
}

void QuadTree::Insert(const int itemIndex, const AxisAlignedBox* itemBounds, const std::size_t itemCount) {
    if (nodes.empty() || itemBounds == nullptr || static_cast<std::size_t>(itemIndex) >= itemCount) {
        return;
    }
    (void)itemCount;
    InsertRecursive(0, itemIndex, itemBounds, 0);
}

void QuadTree::InsertRecursive(
        const int nodeIndex,
        const int itemIndex,
        const AxisAlignedBox* itemBounds,
        const int depth) {
    Node& n = nodes[static_cast<std::size_t>(nodeIndex)];
    const AxisAlignedBox& bounds = itemBounds[static_cast<std::size_t>(itemIndex)];

    if (n.children[0] < 0 && (depth >= treeOptions.maxDepth || static_cast<int>(n.items.size()) < treeOptions.maxLeafSize)) {
        n.items.push_back(itemIndex);
        return;
    }

    if (n.children[0] < 0) {
        const std::vector<int> old = std::move(n.items);
        n.items.clear();
        for (int q = 0; q < 4; ++q) {
            Node child{};
            ChildBounds(n.minX, n.maxX, n.minZ, n.maxZ, q, child.minX, child.maxX, child.minZ, child.maxZ);
            n.children[static_cast<std::size_t>(q)] = static_cast<int>(nodes.size());
            nodes.push_back(child);
        }
        for (const int p : old) {
            InsertRecursive(nodeIndex, p, itemBounds, depth);
        }
    }

    int target = -1;
    if (depth < treeOptions.maxDepth) {
        for (int q = 0; q < 4; ++q) {
            const int ci = n.children[static_cast<std::size_t>(q)];
            const Node& ch = nodes[static_cast<std::size_t>(ci)];
            if (QuadCellFullyContains(ch.minX, ch.maxX, ch.minZ, ch.maxZ, bounds)) {
                target = ci;
                break;
            }
        }
    }
    if (target >= 0) {
        InsertRecursive(target, itemIndex, itemBounds, depth + 1);
    } else {
        n.items.push_back(itemIndex);
    }
}

}  // namespace Spark