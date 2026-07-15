#pragma once

#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class AiBlackboard;

/**
 * Single state in an FSM (Open/Closed: new states subclass without editing the machine).
 */
class IFsmState {
public:
    virtual ~IFsmState() = default;

    virtual void OnEnter(AiBlackboard& board) { (void)board; }
    virtual void OnExit(AiBlackboard& board) { (void)board; }
    virtual void OnTick(const FrameTiming& timing, AiBlackboard& board) = 0;
};

}  // namespace Spark
