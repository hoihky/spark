#include <gtest/gtest.h>

#include "spark/physics/SpatialHashGrid2D.hpp"
#include "spark/physics/colliders/ColliderBakePipeline2D.hpp"
#include "spark/physics/colliders/ColliderBakePipeline3D.hpp"
#include "spark/physics/colliders/IColliderBakeStrategy2D.hpp"
#include "spark/physics/colliders/IColliderBakeStrategy3D.hpp"
#include "spark/scene/GameWorld.hpp"

namespace {

class NeverContributesStrategy2D final : public Spark::IColliderBakeStrategy2D {
public:
    [[nodiscard]] bool Contributes(Spark::GameObject& /*object*/) const noexcept override { return false; }

    void Bake(Spark::GameObject& /*object*/, Spark::ColliderBakeContext2D& /*context*/) const override {
        bakeCount += 1;
    }

    mutable int bakeCount = 0;
};

class NeverContributesStrategy3D final : public Spark::IColliderBakeStrategy3D {
public:
    [[nodiscard]] bool Contributes(Spark::GameObject& /*object*/) const noexcept override { return false; }

    void Bake(Spark::GameObject& /*object*/, Spark::ColliderBakeContext3D& /*context*/) const override {
        bakeCount += 1;
    }

    mutable int bakeCount = 0;
};

}  // namespace

TEST(ColliderBakePipelineTest, DefaultPipelineRebuildsEmptyWorld) {
    Spark::GameWorld world{};
    Spark::Array<Spark::Collider2D> colliders{};
    Spark::SpatialHashGrid2D grid{};

    Spark::ColliderBakePipeline2D::GetDefault().Rebuild(world, 4.0F, colliders, grid);
    EXPECT_EQ(colliders.GetSize(), 0U);
    EXPECT_FLOAT_EQ(grid.GetCellSize(), 4.0F);
}

TEST(ColliderBakePipelineTest, CreateDefaultRegistersFourBuiltInStrategies2D) {
    const Spark::ColliderBakePipeline2D pipeline = Spark::ColliderBakePipeline2D::CreateDefault();
    EXPECT_EQ(pipeline.GetStrategyCount(), 4U);
}

TEST(ColliderBakePipelineTest, CreateDefaultRegistersThreeBuiltInStrategies3D) {
    const Spark::ColliderBakePipeline3D pipeline = Spark::ColliderBakePipeline3D::CreateDefault();
    EXPECT_EQ(pipeline.GetStrategyCount(), 3U);
}

TEST(ColliderBakePipelineTest, RegisterStrategyAppendsToPipeline) {
    Spark::ColliderBakePipeline2D pipeline{};
    EXPECT_EQ(pipeline.GetStrategyCount(), 0U);
    pipeline.RegisterStrategy(Spark::UniquePtr<Spark::IColliderBakeStrategy2D>(new NeverContributesStrategy2D()));
    EXPECT_EQ(pipeline.GetStrategyCount(), 1U);
}

TEST(ColliderBakePipelineTest, RebuildDoesNotBakeWhenContributesIsFalse) {
    Spark::ColliderBakePipeline3D pipeline{};
    auto* strategy = new NeverContributesStrategy3D();
    pipeline.RegisterStrategy(Spark::UniquePtr<Spark::IColliderBakeStrategy3D>(strategy));

    Spark::GameWorld world{};
    Spark::Array<Spark::Collider3D> colliders{};
    Spark::SpatialHashGrid3D grid{};
    pipeline.Rebuild(world, 2.0F, colliders, grid);

    EXPECT_EQ(strategy->bakeCount, 0);
    EXPECT_EQ(colliders.GetSize(), 0U);
}

TEST(ColliderBakePipelineTest, DefaultPipelineRebuildsEmptyWorld) {
    Spark::GameWorld world{};
    Spark::Array<Spark::Collider2D> colliders{};
    Spark::SpatialHashGrid2D grid{};
    Spark::ColliderBakePipeline2D::GetDefault().Rebuild(world, 4.0F, colliders, grid);
    EXPECT_EQ(colliders.GetSize(), 0U);
    EXPECT_FLOAT_EQ(grid.GetCellSize(), 4.0F);
}
