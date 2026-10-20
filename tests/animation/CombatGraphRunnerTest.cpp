#include <gtest/gtest.h>

#include "spark/animation/combat/CombatActionKind.hpp"
#include "spark/animation/combat/CombatActionSpec.hpp"
#include "spark/animation/combat/CombatGraphRunner.hpp"
#include "spark/animation/combat/CombatPhaseKind.hpp"
#include "spark/animation/LocomotionClipSet.hpp"

TEST(CombatGraphRunner, StaggerDoesNotInterruptAttackWindup) {
    Spark::CombatGraphRunner runner{};

    Spark::CombatActionSpec attack{};
    attack.SetKind(Spark::CombatActionKind{Spark::CombatActionKind::Value::Attack});
    attack.SetClipIndex(1);
    attack.SetInterruptPriority(1);
    runner.RegisterAction(attack);

    Spark::CombatActionSpec stagger{};
    stagger.SetKind(Spark::CombatActionKind{Spark::CombatActionKind::Value::Stagger});
    stagger.SetClipIndex(2);
    stagger.SetInterruptPriority(3);
    runner.RegisterAction(stagger);

    EXPECT_TRUE(runner.Request(Spark::CombatActionKind{Spark::CombatActionKind::Value::Attack}));
    runner.AdvanceOverlayPlayback(0.05F, false);
    EXPECT_EQ(runner.GetActivePhase().GetValue(), Spark::CombatPhaseKind::Value::Windup);

    EXPECT_FALSE(runner.Request(Spark::CombatActionKind{Spark::CombatActionKind::Value::Stagger}));
    EXPECT_EQ(runner.GetPendingAction().GetValue(), Spark::CombatActionKind::Value::Stagger);
}

TEST(CombatGraphRunner, StaggerInterruptsDuringRecovery) {
    Spark::CombatGraphRunner runner{};

    Spark::CombatActionSpec attack{};
    attack.SetKind(Spark::CombatActionKind{Spark::CombatActionKind::Value::Attack});
    attack.SetClipIndex(1);
    attack.SetInterruptPriority(1);
    runner.RegisterAction(attack);

    Spark::CombatActionSpec stagger{};
    stagger.SetKind(Spark::CombatActionKind{Spark::CombatActionKind::Value::Stagger});
    stagger.SetClipIndex(2);
    stagger.SetInterruptPriority(3);
    runner.RegisterAction(stagger);

    runner.Request(Spark::CombatActionKind{Spark::CombatActionKind::Value::Attack});
    runner.AdvanceOverlayPlayback(0.8F, false);
    EXPECT_EQ(runner.GetActivePhase().GetValue(), Spark::CombatPhaseKind::Value::Recovery);

    EXPECT_TRUE(runner.Request(Spark::CombatActionKind{Spark::CombatActionKind::Value::Stagger}));
    EXPECT_EQ(runner.GetActiveAction().GetValue(), Spark::CombatActionKind::Value::Stagger);
}
