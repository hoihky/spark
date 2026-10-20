#include <gtest/gtest.h>

#include "spark/animation/locomotion/CardinalDirectionMixer.hpp"
#include "spark/animation/locomotion/DirectionalLocomotionClipCatalog.hpp"

TEST(CardinalDirectionMixer, ForwardUsesForwardClip) {
    Spark::DirectionalLocomotionClipCatalog catalog{};
    catalog.SetForwardWalkClip(10);
    catalog.SetStrafeRightWalkClip(11);

    Spark::CardinalDirectionMixer mixer{};
    mixer.SetCatalog(catalog);

    std::uint32_t clipA = 0;
    std::uint32_t clipB = 0;
    float blend = 0.0F;
    mixer.Evaluate(0.0F, 1.5F, 0.35F, 2.2F, false, clipA, clipB, blend);
    EXPECT_EQ(clipA, 10u);
}
