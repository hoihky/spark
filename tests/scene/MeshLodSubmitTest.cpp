#include <gtest/gtest.h>

#include "spark/core/Array.hpp"
#include "spark/core/SharedPtr.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/MeshLodComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/lod/MeshLodLevel.hpp"
#include "spark/scene/lod/MeshLodSubmit.hpp"

TEST(MeshLodSubmitTest, RigidLodSelectsCustomMeshAtDistance) {
    Spark::MeshComponent mesh{Spark::SharedPtr<Spark::Mesh>{}, Spark::SceneMeshSlot::UnitCube, Spark::Vector3::One};
    auto lodMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String{"LodMesh"});
    Spark::MeshLodComponent lod{};
    Spark::Array<Spark::MeshLodLevel> levels{};
    levels.PushBack(Spark::MeshLodLevel{.minDistance = 0.0F, .meshSlot = Spark::SceneMeshSlot::UnitCube});
    Spark::MeshLodLevel farLod{};
    farLod.minDistance = 50.0F;
    farLod.meshSlot = Spark::SceneMeshSlot::Custom;
    farLod.rigidMesh = lodMesh;
    farLod.paletteUpdateStride = 4U;
    levels.PushBack(farLod);
    lod.GetLevels() = levels;
    lod.SetHysteresisMeters(0.0F);

    const Spark::Matrix4 world = Spark::Matrix4::Translation({0.0F, 0.0F, 60.0F});
    const Spark::MeshLodResolvedDraw nearDraw =
            Spark::MeshLodResolveRigidDraw(mesh, &lod, {0.0F, 0.0F, 0.0F}, world);
    EXPECT_EQ(nearDraw.lodIndex, 1U);
    EXPECT_EQ(nearDraw.meshSlot, Spark::SceneMeshSlot::Custom);
    EXPECT_EQ(nearDraw.rigidMesh.Get(), lodMesh.Get());
    EXPECT_EQ(nearDraw.paletteUpdateStride, 4U);
}