#include "spark/scene/spatial/OctTree.hpp"

namespace Spark {

void OctTree::Clear() noexcept {
    nodes.clear();
}

void OctTree::Reset(const AxisAlignedBox& rootBounds, Options options) {
    nodes.clear();
    treeOptions = options;
    Node root{};
    root.cell = rootBounds;
    nodes.push_back(root);
}

AxisAlignedBox OctTree::OctantOf(const AxisAlignedBox& parent, const int octantIndex) noexcept {
    const Vector3 c = (parent.min + parent.max) * 0.5F;
    Vector3 mn = parent.min;
    Vector3 mx = parent.max;
    if ((octantIndex & 1) != 0) {
        mn.x = c.x;
    } else {
        mx.x = c.x;
    }
    if ((octantIndex & 2) != 0) {
        mn.y = c.y;
    } else {
        mx.y = c.y;
    }
    if ((octantIndex & 4) != 0) {
        mn.z = c.z;
    } else {
        mx.z = c.z;
    }
    return AxisAlignedBox{mn, mx};
}

bool OctTree::CellFullyContains(const AxisAlignedBox& cell, const AxisAlignedBox& item) noexcept {
    return item.min.x >= cell.min.x && item.max.x <= cell.max.x && item.min.y >= cell.min.y && item.max.y <= cell.max.y &&
            item.min.z >= cell.min.z && item.max.z <= cell.max.z;
}

void OctTree::Insert(const int itemIndex, const AxisAlignedBox* itemBounds, const std::size_t itemCount) {
    if (nodes.empty() || itemBounds == nullptr || static_cast<std::size_t>(itemIndex) >= itemCount) {
        return;
    }
    (void)itemCount;
    InsertRecursive(0, itemIndex, itemBounds, 0);
}

void OctTree::InsertRecursive(
        const int nodeIndex,
        const int itemIndex,
        const AxisAlignedBox* itemBounds,
        const int depth) {
    Node& n = nodes[static_cast<std::size_t>(nodeIndex)];
    const AxisAlignedBox& bounds = itemBounds[itemIndex];

    if (n.children[0] < 0 && (depth >= treeOptions.maxDepth || static_cast<int>(n.items.size()) < treeOptions.maxLeafSize)) {
        n.items.push_back(itemIndex);
        return;
    }

    if (n.children[0] < 0) {
        const std::vector<int> old = std::move(n.items);
        n.items.clear();
        for (int o = 0; o < 8; ++o) {
            Node child{};
            child.cell = OctantOf(n.cell, o);
            n.children[static_cast<std::size_t>(o)] = static_cast<int>(nodes.size());
            nodes.push_back(child);
        }
        for (const int p : old) {
            InsertRecursive(nodeIndex, p, itemBounds, depth);
        }
    }

    int targetChild = -1;
    if (depth < treeOptions.maxDepth) {
        for (int o = 0; o < 8; ++o) {
            const int ci = n.children[static_cast<std::size_t>(o)];
            if (CellFullyContains(nodes[static_cast<std::size_t>(ci)].cell, bounds)) {
                targetChild = ci;
                break;
            }
        }
    }
    if (targetChild >= 0) {
        InsertRecursive(targetChild, itemIndex, itemBounds, depth + 1);
    } else {
        n.items.push_back(itemIndex);
    }
}

}  // namespace Spark