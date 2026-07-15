#include "spark/ai/goap/GoapPlanner.hpp"

namespace Spark {

namespace {

[[nodiscard]] bool GoalSatisfied(const std::uint64_t w, const std::uint64_t goalMask, const std::uint64_t goalValue) noexcept {
    return (w & goalMask) == (goalValue & goalMask);
}

[[nodiscard]] bool PreconditionsHold(const std::uint64_t w, const GoapActionSpec& a) noexcept {
    return (w & a.preMask) == (a.preValue & a.preMask);
}

[[nodiscard]] std::uint64_t ApplyEffects(const std::uint64_t w, const GoapActionSpec& a) noexcept {
    return (w | a.effectSetMask) & ~a.effectClearMask;
}

[[nodiscard]] bool ContainsWorld(const Array<std::uint64_t>& set, const std::uint64_t w) noexcept {
    for (std::size_t i = 0; i < set.GetSize(); ++i) {
        if (set[i] == w) {
            return true;
        }
    }
    return false;
}

struct SearchNode {
    std::uint64_t world = 0;
    std::size_t parent = 0;
    std::uint32_t actionFromParent = 0xFFFFFFFFu;
};

}  // namespace

bool GoapPlanner::Plan(
        const std::uint64_t startWorld,
        const std::uint64_t goalMask,
        const std::uint64_t goalValue,
        const Array<GoapActionSpec>& actions,
        Array<std::uint32_t>& outPlan) {
    outPlan.Clear();
    if (GoalSatisfied(startWorld, goalMask, goalValue)) {
        return true;
    }

    Array<SearchNode> nodes;
    Array<std::uint64_t> expanded;

    SearchNode root{};
    root.world = startWorld;
    root.parent = static_cast<std::size_t>(-1);
    root.actionFromParent = 0xFFFFFFFFu;
    nodes.PushBack(root);

    for (std::size_t qi = 0; qi < nodes.GetSize(); ++qi) {
        const std::uint64_t w = nodes[qi].world;
        if (ContainsWorld(expanded, w)) {
            continue;
        }
        expanded.PushBack(w);

        if (GoalSatisfied(w, goalMask, goalValue)) {
            Array<std::uint32_t> rev;
            std::size_t idx = qi;
            while (idx != 0) {
                rev.PushBack(nodes[idx].actionFromParent);
                idx = nodes[idx].parent;
            }
            for (std::size_t i = rev.GetSize(); i > 0; --i) {
                outPlan.PushBack(rev[i - 1]);
            }
            return true;
        }

        for (std::uint32_t ai = 0; ai < static_cast<std::uint32_t>(actions.GetSize()); ++ai) {
            const GoapActionSpec& a = actions[ai];
            if (!PreconditionsHold(w, a)) {
                continue;
            }
            const std::uint64_t w2 = ApplyEffects(w, a);
            if (ContainsWorld(expanded, w2)) {
                continue;
            }
            SearchNode n{};
            n.world = w2;
            n.parent = qi;
            n.actionFromParent = ai;
            nodes.PushBack(n);
        }
    }

    return false;
}

}  // namespace Spark
