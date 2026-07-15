#include "spark/ai/fsm/FiniteStateMachine.hpp"

#include "spark/ai/AiBlackboard.hpp"
#include "spark/core/Utility.hpp"
#include "spark/engine/FrameTiming.hpp"

namespace Spark {

void FsmStateMachine::AddState(UniquePtr<IFsmState> state) {
    states.PushBack(MoveTemp(state));
}

void FsmStateMachine::AddTransition(const FsmTransition& rule) {
    transitions.PushBack(rule);
}

void FsmStateMachine::SetInitialState(const std::uint32_t stateId) noexcept {
    initial = stateId;
    if (!booted) {
        current = stateId;
    }
}

IFsmState* FsmStateMachine::StateAt_(const std::uint32_t id) const noexcept {
    if (id >= states.GetSize()) {
        return nullptr;
    }
    return states[id].Get();
}

void FsmStateMachine::EnterState_(const std::uint32_t next, AiBlackboard& board) {
    if (IFsmState* prev = StateAt_(current)) {
        prev->OnExit(board);
    }
    current = next;
    if (IFsmState* s = StateAt_(current)) {
        s->OnEnter(board);
    }
}

bool FsmStateMachine::SendEvent(const std::uint32_t eventId, AiBlackboard& board) {
    for (std::size_t i = 0; i < transitions.GetSize(); ++i) {
        const FsmTransition& t = transitions[i];
        if (t.fromState == current && t.eventId == eventId) {
            EnterState_(t.toState, board);
            return true;
        }
    }
    return false;
}

void FsmStateMachine::Tick(const FrameTiming& timing, AiBlackboard& board) {
    if (!booted) {
        booted = true;
        if (IFsmState* s = StateAt_(current)) {
            s->OnEnter(board);
        }
    }
    if (IFsmState* s = StateAt_(current)) {
        s->OnTick(timing, board);
    }
}

}  // namespace Spark
