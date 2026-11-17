#pragma once

#include "spark/math/AxisAlignedBox.hpp"
#include "spark/math/Frustum.hpp"
#include "spark/math/Vector3.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace Spark {

struct QuadTreeOptions {
    int maxDepth = 16;
    int maxLeafSize = 10;
};

/**
 * Vertical quadtree on world XZ; leaf tests use full 3D item bounds.
 * Cell frustum tests use an infinitely tall XZ slab (±1e9 on Y).
 */
class QuadTree final {
public:
    using Options = QuadTreeOptions;

    void Clear() noexcept;
    void Reset(float minX, float maxX, float minZ, float maxZ, Options options = QuadTreeOptions());

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
        float minX = 0.0F;
        float maxX = 0.0F;
        float minZ = 0.0F;
        float maxZ = 0.0F;
        std::vector<int> items;
        std::array<int, 4> children{};
        Node() { children.fill(-1); }
    };

    [[nodiscard]] static bool QuadCellFullyContains(
            float minX,
            float maxX,
            float minZ,
            float maxZ,
            const AxisAlignedBox& item) noexcept;

    static void ChildBounds(
            float pminX,
            float pmaxX,
            float pminZ,
            float pmaxZ,
            int quadrant,
            float& cminX,
            float& cmaxX,
            float& cminZ,
            float& cmaxZ) noexcept;

    void InsertRecursive(int nodeIndex, int itemIndex, const AxisAlignedBox* itemBounds, int depth);

    template<typename Fn>
    void QueryFrustumRecursive(int nodeIndex, const Frustum& frustum, const AxisAlignedBox* itemBounds, Fn&& onItem)
            const {
        if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes.size())) {
            return;
        }
        const Node& n = nodes[static_cast<std::size_t>(nodeIndex)];
        const Vector3 qmn{n.minX, -1.0e9F, n.minZ};
        const Vector3 qmx{n.maxX, 1.0e9F, n.maxZ};
        if (!frustum.IntersectsAxisAlignedBox(qmn, qmx)) {
            return;
        }
        for (const int item : n.items) {
            const AxisAlignedBox& box = itemBounds[static_cast<std::size_t>(item)];
            if (frustum.Intersects(box)) {
                onItem(item);
            }
        }
        for (int q = 0; q < 4; ++q) {
            const int ci = n.children[static_cast<std::size_t>(q)];
            if (ci >= 0) {
                QueryFrustumRecursive(ci, frustum, itemBounds, onItem);
            }
        }
    }

    std::vector<Node> nodes{};
    Options treeOptions{};
};

}  // namespace Spark