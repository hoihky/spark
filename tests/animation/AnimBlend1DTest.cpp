#include "spark/animation/AnimBlend1D.hpp"

#include <gtest/gtest.h>

namespace {

TEST(AnimBlend1DTest, EmptyKeyframesAreInvalid) {
    const Spark::AnimBlend1DSample sample = Spark::EvaluateAnimBlend1D(nullptr, 0, 1.0F);
    EXPECT_FALSE(sample.valid);
}

TEST(AnimBlend1DTest, SingleKeyframeAlwaysReturnsThatClip) {
    const Spark::AnimBlend1DKeyframe keys[] = {{1.5F, 7}};
    const Spark::AnimBlend1DSample low = Spark::EvaluateAnimBlend1D(keys, 1, 0.0F);
    const Spark::AnimBlend1DSample high = Spark::EvaluateAnimBlend1D(keys, 1, 9.0F);
    EXPECT_TRUE(low.valid);
    EXPECT_EQ(low.clipA, 7U);
    EXPECT_EQ(low.clipB, 7U);
    EXPECT_FLOAT_EQ(low.blend01, 0.0F);
    EXPECT_TRUE(high.valid);
    EXPECT_EQ(high.clipA, 7U);
}

TEST(AnimBlend1DTest, InterpolatesBetweenThresholds) {
    const Spark::AnimBlend1DKeyframe keys[] = {
            {0.0F, 0},
            {2.0F, 1},
            {4.0F, 2},
    };

    const Spark::AnimBlend1DSample atStart = Spark::EvaluateAnimBlend1D(keys, 3, 0.0F);
    EXPECT_EQ(atStart.clipA, 0U);
    EXPECT_EQ(atStart.clipB, 0U);
    EXPECT_FLOAT_EQ(atStart.blend01, 0.0F);

    const Spark::AnimBlend1DSample mid = Spark::EvaluateAnimBlend1D(keys, 3, 1.0F);
    EXPECT_EQ(mid.clipA, 0U);
    EXPECT_EQ(mid.clipB, 1U);
    EXPECT_FLOAT_EQ(mid.blend01, 0.5F);

    const Spark::AnimBlend1DSample atEnd = Spark::EvaluateAnimBlend1D(keys, 3, 4.0F);
    EXPECT_EQ(atEnd.clipA, 2U);
    EXPECT_EQ(atEnd.clipB, 2U);
    EXPECT_FLOAT_EQ(atEnd.blend01, 0.0F);
}

}  // namespace
