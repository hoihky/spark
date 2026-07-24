# Finite State Machines

## Class Design: `FsmStateMachine`

```cpp
struct FsmTransition {
    std::uint32_t fromState;
    std::uint32_t eventId;
    std::uint32_t toState;
};

class FsmStateMachine {
public:
    void AddState(UniquePtr<IFsmState> state);
    void AddTransition(const FsmTransition& rule);
    void SetInitialState(std::uint32_t stateId) noexcept;
    bool SendEvent(std::uint32_t eventId, AiBlackboard& board);
    void Tick(const FrameTiming& timing, AiBlackboard& board);
};
```

## Implement a State

```cpp
class PatrolState final : public IFsmState {
public:
  explicit PatrolState(std::uint32_t id) : stateId(id) {}
  std::uint32_t GetId() const noexcept override { return stateId; }

  void OnEnter(AiBlackboard& board) override { (void)board; }
  void OnExit(AiBlackboard& board) override { (void)board; }
  void Tick(const FrameTiming& timing, AiBlackboard& board) override {
    (void)timing; (void)board;
    // wander, play anim, etc.
  }
private:
  std::uint32_t stateId;
};
```

## Wire FSM to Agent

```cpp
enum : std::uint32_t { kIdle = 1, kChase = 2, kAttack = 3 };
enum : std::uint32_t { kEvtSeePlayer = 100, kEvtLostPlayer = 101 };

auto fsm = MakeUnique<FsmStateMachine>();
fsm->AddState(MakeUnique<IdleState>(kIdle));
fsm->AddState(MakeUnique<ChaseState>(kChase));
fsm->AddTransition({kIdle, kEvtSeePlayer, kChase});
fsm->AddTransition({kChase, kEvtLostPlayer, kIdle});
fsm->SetInitialState(kIdle);

agent->SetFsmEnabled(true);
agent->SetFsm(MoveTemp(fsm));
```

## Events from Gameplay

```cpp
if (distanceToPlayer < 12.0F)
    agent->TryGetFsm()->SendEvent(kEvtSeePlayer, agent->GetBlackboard());
```

`AiAgentComponent::SubsystemTick` calls `fsm->Tick` when `fsmEnabled`.

Next: [GOAP](04-goap.md).
