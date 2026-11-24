#include <gtest/gtest.h>

#include "spark/scene/submit/SkinnedAnimationBudget.hpp"

TEST(SkinnedAnimationBudgetCountersTest, PaletteUpdateCapResetsEachFrame) {
    Spark::SkinnedAnimationBudget budget{};
    budget.SetMaxPaletteUpdatesPerFrame(1);
    budget.BeginFrame();
    EXPECT_TRUE(budget.ConsumePaletteUpdate());
    EXPECT_FALSE(budget.ConsumePaletteUpdate());
    EXPECT_EQ(budget.GetPaletteUpdatesUsed(), 1U);

    budget.BeginFrame();
    EXPECT_TRUE(budget.ConsumePaletteUpdate());
    EXPECT_EQ(budget.GetPaletteUpdatesUsed(), 1U);
}

TEST(SkinnedAnimationBudgetCountersTest, SkinnedDrawCapIsEnforced) {
    Spark::SkinnedAnimationBudget budget{};
    budget.SetMaxSkinnedDrawsPerFrame(1);
    budget.BeginFrame();
    EXPECT_TRUE(budget.ConsumeSkinnedDraw());
    EXPECT_FALSE(budget.ConsumeSkinnedDraw());
    EXPECT_EQ(budget.GetSkinnedDrawsSubmitted(), 1U);
}