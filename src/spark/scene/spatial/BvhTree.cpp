#include "spark/scene/spatial/BvhTree.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 Centroid(const AxisAlignedBox& box) noexcept {
    return (box.min + box.max) * 0.5F;
}

}  // namespace

void BvhTree::Clear() noexcept {
    itemOrder.clear();
    nodes.clear();
    itemCount = 0;
    lastFingerprint = 0;
}

void BvhTree::Build(const AxisAlignedBox* bounds, const std::size_t count, BuildOptions options) {
    Clear();
    if (bounds == nullptr || count == 0) {
        return;
    }
    itemCount = count;
    itemOrder.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        itemOrder[i] = static_cast<int>(i);
    }
    nodes.clear();
    nodes.reserve(count * 2);
    BuildRecursive(bounds, 0, static_cast<int>(count), 0, options);
}

void BvhTree::BuildIfChanged(
        const AxisAlignedBox* bounds,
        const std::size_t count,
        const std::uint64_t userFingerprint,
        BuildOptions options) {
    if (count == itemCount && userFingerprint == lastFingerprint && !nodes.empty()) {
        return;
    }
    lastFingerprint = userFingerprint;
    Build(bounds, count, options);
}

AxisAlignedBox BvhTree::BoundsOfRange(
        const AxisAlignedBox* bounds,
        const std::vector<int>& ids,
        const int first,
        const int count) noexcept {
    const int i0 = ids[static_cast<std::size_t>(first)];
    AxisAlignedBox b = bounds[static_cast<std::size_t>(i0)];
    for (int i = 1; i < count; ++i) {
        const int idx = ids[static_cast<std::size_t>(first + i)];
        b = AxisAlignedBox::UnionOf(b, bounds[static_cast<std::size_t>(idx)]);
    }
    return b;
}

int BvhTree::BuildRecursive(
        const AxisAlignedBox* bounds,
        const int first,
        const int count,
        const int depth,
        BuildOptions options) {
    const int nodeIndex = static_cast<int>(nodes.size());
    Node node{};
    node.first = first;
    node.count = count;
    node.bounds = BoundsOfRange(bounds, itemOrder, first, count);
    nodes.push_back(node);

    if (count <= options.maxLeafSize || depth >= options.maxDepth) {
        return nodeIndex;
    }

    int axis = 0;
    if (options.splitPolicy == SplitPolicy::LongestExtent) {
        const Vector3 e = node.bounds.max - node.bounds.min;
        axis = (e.x >= e.y && e.x >= e.z) ? 0 : (e.y >= e.z ? 1 : 2);
    } else {
        axis = depth % 3;
    }

    const int begin = first;
    const int end = first + count;
    const int midOff = count / 2;
    std::nth_element(itemOrder.begin() + begin, itemOrder.begin() + begin + midOff, itemOrder.begin() + end,
            [bounds, axis](const int a, const int b) {
                const Vector3 ca3 = Centroid(bounds[static_cast<std::size_t>(a)]);
                const Vector3 cb3 = Centroid(bounds[static_cast<std::size_t>(b)]);
                const float ca = axis == 0 ? ca3.x : (axis == 1 ? ca3.y : ca3.z);
                const float cb = axis == 0 ? cb3.x : (axis == 1 ? cb3.y : cb3.z);
                if (ca < cb) {
                    return true;
                }
                if (cb < ca) {
                    return false;
                }
                return a < b;
            });
    const int mid = first + count / 2;
    const int leftCount = mid - first;
    const int rightCount = count - leftCount;
    if (leftCount == 0 || rightCount == 0) {
        return nodeIndex;
    }

    const int L = BuildRecursive(bounds, first, leftCount, depth + 1, options);
    const int R = BuildRecursive(bounds, mid, rightCount, depth + 1, options);
    nodes[static_cast<std::size_t>(nodeIndex)].left = L;
    nodes[static_cast<std::size_t>(nodeIndex)].right = R;
    nodes[static_cast<std::size_t>(nodeIndex)].count = 0;
    return nodeIndex;
}

}  // namespace Spark