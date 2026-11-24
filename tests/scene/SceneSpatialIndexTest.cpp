#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <vector>

#include "spark/math/Frustum.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/spatial/BvhTree.hpp"
#include "spark/scene/spatial/OctTree.hpp"
#include "spark/scene/spatial/QuadTree.hpp"
#include "spark/scene/spatial/SpatialBoundsUtil.hpp"

namespace {

[[nodiscard]] Spark::Frustum MakeTestFrustum() {
    const Spark::Matrix4 proj = Spark::Matrix4::PerspectiveVulkan(1.0471975F, 16.0F / 9.0F, 0.1F, 500.0F);
    const Spark::Matrix4 view =
            Spark::Matrix4::LookAt({0.0F, 2.0F, 8.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    return Spark::Frustum::FromColumnMajorViewProjection(proj * view);
}

[[nodiscard]] Spark::AxisAlignedBox BoxAt(const float x, const float z, const float half = 0.5F) {
    return Spark::AxisAlignedBox::FromMinMax(
            {x - half, -half, z - half},
            {x + half, half, z + half});
}

void CollectBruteForce(
        const Spark::Frustum& frustum,
        const Spark::AxisAlignedBox* bounds,
        const std::size_t count,
        std::vector<int>& out) {
    out.clear();
    for (std::size_t i = 0; i < count; ++i) {
        if (frustum.Intersects(bounds[i])) {
            out.push_back(static_cast<int>(i));
        }
    }
    std::sort(out.begin(), out.end());
}

template<typename TreeFn>
void ExpectMatchesBruteForce(
        const Spark::Frustum& frustum,
        const std::array<Spark::AxisAlignedBox, 9>& bounds,
        TreeFn&& runTreeQuery) {
    std::vector<int> expected;
    CollectBruteForce(frustum, bounds.data(), bounds.size(), expected);

    std::vector<int> actual;
    runTreeQuery([&](const int index) { actual.push_back(index); });
    std::sort(actual.begin(), actual.end());

    EXPECT_EQ(actual, expected);
}

}  // namespace

TEST(SceneSpatialIndexTest, BvhFrustumQueryMatchesBruteForce) {
    const Spark::Frustum frustum = MakeTestFrustum();
    const std::array<Spark::AxisAlignedBox, 9> bounds = {
            BoxAt(-12.0F, -12.0F),
            BoxAt(0.0F, -12.0F),
            BoxAt(12.0F, -12.0F),
            BoxAt(-12.0F, 0.0F),
            BoxAt(0.0F, 0.0F),
            BoxAt(12.0F, 0.0F),
            BoxAt(-12.0F, 12.0F),
            BoxAt(0.0F, 12.0F),
            BoxAt(12.0F, 12.0F),
    };

    Spark::BvhTree tree;
    tree.Build(bounds.data(), bounds.size());

    ExpectMatchesBruteForce(frustum, bounds, [&](auto&& onItem) {
        tree.QueryFrustum(frustum, bounds.data(), onItem);
    });
    EXPECT_EQ(tree.ItemCount(), bounds.size());
}

TEST(SceneSpatialIndexTest, BvhBuildIfChangedSkipsIdenticalFingerprint) {
    const std::array<Spark::AxisAlignedBox, 3> bounds = {
            BoxAt(0.0F, 0.0F),
            BoxAt(4.0F, 0.0F),
            BoxAt(8.0F, 0.0F),
    };
    const std::uint64_t fp = Spark::SpatialBoundsUtil::FingerprintAxisAlignedBoxes(bounds.data(), bounds.size());

    Spark::BvhTree tree;
    tree.BuildIfChanged(bounds.data(), bounds.size(), fp);
    EXPECT_FALSE(tree.IsEmpty());
    const std::size_t nodeCountAfterFirst = tree.ItemCount();

    tree.BuildIfChanged(bounds.data(), bounds.size(), fp);
    EXPECT_EQ(tree.ItemCount(), nodeCountAfterFirst);
}

TEST(SceneSpatialIndexTest, OctTreeFrustumQueryMatchesBruteForce) {
    const Spark::Frustum frustum = MakeTestFrustum();
    const std::array<Spark::AxisAlignedBox, 9> bounds = {
            BoxAt(-12.0F, -12.0F),
            BoxAt(0.0F, -12.0F),
            BoxAt(12.0F, -12.0F),
            BoxAt(-12.0F, 0.0F),
            BoxAt(0.0F, 0.0F),
            BoxAt(12.0F, 0.0F),
            BoxAt(-12.0F, 12.0F),
            BoxAt(0.0F, 12.0F),
            BoxAt(12.0F, 12.0F),
    };

    Spark::OctTree tree;
    tree.Reset(Spark::SpatialBoundsUtil::UnionSceneItemsPadded(bounds.data(), bounds.size()));
    for (int i = 0; i < static_cast<int>(bounds.size()); ++i) {
        tree.Insert(i, bounds.data(), bounds.size());
    }

    ExpectMatchesBruteForce(frustum, bounds, [&](auto&& onItem) {
        tree.QueryFrustum(frustum, bounds.data(), onItem);
    });
}

TEST(SceneSpatialIndexTest, QuadTreeFrustumQueryMatchesBruteForce) {
    const Spark::Frustum frustum = MakeTestFrustum();
    const std::array<Spark::AxisAlignedBox, 9> bounds = {
            BoxAt(-12.0F, -12.0F),
            BoxAt(0.0F, -12.0F),
            BoxAt(12.0F, -12.0F),
            BoxAt(-12.0F, 0.0F),
            BoxAt(0.0F, 0.0F),
            BoxAt(12.0F, 0.0F),
            BoxAt(-12.0F, 12.0F),
            BoxAt(0.0F, 12.0F),
            BoxAt(12.0F, 12.0F),
    };

    Spark::QuadTree tree;
    tree.Reset(-20.0F, 20.0F, -20.0F, 20.0F);
    for (int i = 0; i < static_cast<int>(bounds.size()); ++i) {
        tree.Insert(i, bounds.data(), bounds.size());
    }

    ExpectMatchesBruteForce(frustum, bounds, [&](auto&& onItem) {
        tree.QueryFrustum(frustum, bounds.data(), onItem);
    });
}