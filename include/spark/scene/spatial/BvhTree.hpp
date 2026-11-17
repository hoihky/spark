#pragma once

#include "spark/math/AxisAlignedBox.hpp"
#include "spark/math/Frustum.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Spark {

enum class BvhSplitPolicy : std::uint8_t {
    LongestExtent = 0,
    CyclicByDepth = 1,
};

struct BvhTreeBuildOptions {
    int maxLeafSize = 4;
    int maxDepth = 24;
    BvhSplitPolicy splitPolicy = BvhSplitPolicy::LongestExtent;
};

/** Binary BVH over indexed axis-aligned bounds; build + frustum query. */
class BvhTree final {
public:
    using BuildOptions = BvhTreeBuildOptions;
    using SplitPolicy = BvhSplitPolicy;

    void Clear() noexcept;

    /** Item <c>i</c> uses <c>bounds[i]</c>. Indices 0 .. count-1 are indexed. */
    void Build(const AxisAlignedBox* bounds, std::size_t count, BuildOptions options = BvhTreeBuildOptions());

    /**
     * Rebuilds only when count or fingerprint changes (for per-frame scene caches).
     * <c>userFingerprint</c> should change when any bound moves (e.g. object pointers + AABBs).
     */
    void BuildIfChanged(
            const AxisAlignedBox* bounds,
            std::size_t count,
            std::uint64_t userFingerprint,
            BuildOptions options = BvhTreeBuildOptions());

    template<typename Fn>
    void QueryFrustum(const Frustum& frustum, const AxisAlignedBox* bounds, Fn&& onItem) const {
        if (bounds == nullptr || nodes.empty()) {
            return;
        }
        QueryFrustumRecursive(0, frustum, bounds, onItem);
    }

    [[nodiscard]] bool IsEmpty() const noexcept { return nodes.empty(); }
    [[nodiscard]] std::size_t ItemCount() const noexcept { return itemCount; }

private:
    struct Node {
        AxisAlignedBox bounds{};
        int left = -1;
        int right = -1;
        int first = 0;
        int count = 0;
    };

    template<typename Fn>
    void QueryFrustumRecursive(int nodeIndex, const Frustum& frustum, const AxisAlignedBox* bounds, Fn&& onItem) const {
        if (nodeIndex < 0) {
            return;
        }
        const Node& n = nodes[static_cast<std::size_t>(nodeIndex)];
        if (!frustum.Intersects(n.bounds)) {
            return;
        }
        if (n.count > 0) {
            for (int i = 0; i < n.count; ++i) {
                const int item = itemOrder[static_cast<std::size_t>(n.first + i)];
                const AxisAlignedBox& box = bounds[static_cast<std::size_t>(item)];
                if (frustum.Intersects(box)) {
                    onItem(item);
                }
            }
            return;
        }
        QueryFrustumRecursive(n.left, frustum, bounds, onItem);
        QueryFrustumRecursive(n.right, frustum, bounds, onItem);
    }

    int BuildRecursive(
            const AxisAlignedBox* bounds,
            int first,
            int count,
            int depth,
            BuildOptions options);

    [[nodiscard]] static AxisAlignedBox BoundsOfRange(
            const AxisAlignedBox* bounds,
            const std::vector<int>& ids,
            int first,
            int count) noexcept;

    std::vector<int> itemOrder{};
    std::vector<Node> nodes{};
    std::size_t itemCount = 0;
    std::uint64_t lastFingerprint = 0;
};

}  // namespace Spark