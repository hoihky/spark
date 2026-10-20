#include <gtest/gtest.h>

#include "spark/animation/AnimationJointMask.hpp"
#include "spark/animation/Skeleton.hpp"

TEST(AnimationJointMaskTest, MissingSpineJointReturnsFalse) {
    Spark::Skeleton skeleton{};
    Spark::AnimationJointMask mask{};
    EXPECT_FALSE(mask.TryBuildUpperBodyFromSpineName(skeleton, "spine"));
}
