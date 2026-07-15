#include "spark/ai/fuzzy/FuzzyLogic.hpp"

#include "spark/ai/AiBlackboard.hpp"

namespace Spark {

float FuzzyTriangle::Membership(const float x) const noexcept {
    if (x <= left || x >= right) {
        return 0.0F;
    }
    if (x < peak) {
        return (peak - left) > 1.0e-8F ? (x - left) / (peak - left) : 1.0F;
    }
    if (x > peak) {
        return (right - peak) > 1.0e-8F ? (right - x) / (right - peak) : 1.0F;
    }
    return 1.0F;
}

void FuzzyAdvisoryModule::Evaluate(AiBlackboard& board) const noexcept {
    const float x = board.GetFloat(inputSlot);
    const float mLow = low.Membership(x);
    const float mHigh = high.Membership(x);
    const float fire = mLow < mHigh ? mLow : mHigh;
    /** Crisp centroid of two singleton outputs at 0 and 1 weighted by complementary memberships. */
    const float denom = mLow + mHigh + 1.0e-6F;
    const float out = (mHigh * 1.0F + mLow * 0.0F) / denom;
    board.SetFloat(outputSlot, out * fire);
}

}  // namespace Spark
