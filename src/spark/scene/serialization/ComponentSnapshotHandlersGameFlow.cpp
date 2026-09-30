#include "spark/scene/serialization/ComponentSnapshotHandlersGameFlow.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/gameplay/GameFlowTriggerComponent.hpp"
#include "spark/ecs/components/gameplay/GameStateComponent.hpp"
#include "spark/ecs/components/input/InputActionMapComponent.hpp"
#include "spark/input/InputAction.hpp"
#include "spark/ecs/components/input/PlayerInputComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectSpawnComponent.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"

#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

bool KindTagEquals(const Utf8String& kind, const char* tag) noexcept {
    return tag != nullptr && std::strcmp(kind.CStr(), tag) == 0;
}

template<typename HandlerT>
void RegisterHandler(ComponentSnapshotRegistry& registry) {
    UniquePtr<HandlerT> concrete = MakeUnique<HandlerT>();
    registry.Register(UniquePtr<IComponentSnapshotHandler>(
            static_cast<IComponentSnapshotHandler*>(concrete.Release())));
}

class GameStateSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::GameState; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "game_state"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const GameStateComponent* state = owner.GetComponent<GameStateComponent>();
        if (state == nullptr) {
            return false;
        }
        char buf[256]{};
        std::snprintf(buf, sizeof(buf), "%u 0", static_cast<unsigned>(state->GetState()));
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        unsigned current = 0U;
        unsigned stackDepth = 0U;
        if (std::sscanf(record.payload.CStr(), "%u %u", &current, &stackDepth) < 1) {
            return false;
        }
        GameStateComponent* state = owner.GetComponent<GameStateComponent>();
        if (state == nullptr) {
            state = owner.AddComponent<GameStateComponent>(static_cast<GameFlowState>(current));
        } else {
            state->RequestState(static_cast<GameFlowState>(current));
        }
        return true;
    }
};

class GameFlowTriggerSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::GameFlowTrigger; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "game_flow_trigger"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const GameFlowTriggerComponent* trigger = owner.GetComponent<GameFlowTriggerComponent>();
        if (trigger == nullptr) {
            return false;
        }
        char buf[512]{};
        std::snprintf(
                buf,
                sizeof(buf),
                "%u %u %u \"%s\"",
                static_cast<unsigned>(trigger->GetSource()),
                static_cast<unsigned>(trigger->GetTargetState()),
                static_cast<unsigned>(trigger->GetWatchState()),
                trigger->GetInstigatorNameFilter());
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        unsigned source = 0U;
        unsigned target = 0U;
        unsigned watch = 0U;
        char filter[256]{};
        if (std::sscanf(record.payload.CStr(), "%u %u %u \"%255[^\"]\"", &source, &target, &watch, filter) < 3) {
            return false;
        }
        GameFlowTriggerComponent* trigger = owner.GetComponent<GameFlowTriggerComponent>();
        if (trigger == nullptr) {
            trigger = owner.AddComponent<GameFlowTriggerComponent>();
        }
        trigger->SetSource(static_cast<GameFlowTriggerSource>(source));
        trigger->SetTargetState(static_cast<GameFlowState>(target));
        trigger->SetWatchState(static_cast<GameFlowState>(watch));
        trigger->SetInstigatorNameFilter(filter);
        return true;
    }
};

class InputActionMapSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::InputActionMap; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "input_action_map"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const InputActionMapComponent* map = owner.GetComponent<InputActionMapComponent>();
        if (map == nullptr) {
            return false;
        }
        Utf8String payload{};
        const Array<UniquePtr<InputAction>>& actions = map->GetActions();
        char line[256]{};
        std::snprintf(line, sizeof(line), "%zu", actions.GetSize());
        payload.AppendUtf8(line);
        for (std::size_t i = 0; i < actions.GetSize(); ++i) {
            if (actions[i] == nullptr) {
                continue;
            }
            const InputAction& action = *actions[i];
            std::snprintf(
                    line,
                    sizeof(line),
                    " \"%s\" %u %d %d %d %d %d %d",
                    action.GetName().CStr(),
                    static_cast<unsigned>(action.GetType()),
                    action.GetLegacyPrimaryKey(),
                    action.GetLegacySecondaryKey(),
                    action.GetLegacyNegativeKey(),
                    action.GetLegacyPositiveKey(),
                    action.GetLegacySecondaryNegativeKey(),
                    action.GetLegacySecondaryPositiveKey());
            payload.AppendUtf8(line);
        }
        out.kind = Utf8String(GetKindTag());
        out.payload = payload;
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        std::size_t count = 0U;
        const char* cursor = record.payload.CStr();
        if (std::sscanf(cursor, "%zu", &count) != 1) {
            return false;
        }
        while (*cursor != '\0' && *cursor != ' ') {
            ++cursor;
        }
        InputActionMapComponent* map = owner.GetComponent<InputActionMapComponent>();
        if (map == nullptr) {
            map = owner.AddComponent<InputActionMapComponent>();
        }
        map->ClearActions();
        for (std::size_t i = 0; i < count; ++i) {
            while (*cursor == ' ') {
                ++cursor;
            }
            if (*cursor != '"') {
                return false;
            }
            ++cursor;
            char name[128]{};
            std::size_t ni = 0;
            while (*cursor != '\0' && *cursor != '"' && ni + 1 < sizeof(name)) {
                name[ni++] = *cursor++;
            }
            name[ni] = '\0';
            if (*cursor == '"') {
                ++cursor;
            }
            unsigned type = 0U;
            int keys[6]{-1, -1, -1, -1, -1, -1};
            if (std::sscanf(
                        cursor,
                        " %u %d %d %d %d %d %d",
                        &type,
                        &keys[0],
                        &keys[1],
                        &keys[2],
                        &keys[3],
                        &keys[4],
                        &keys[5]) < 2) {
                return false;
            }
            if (static_cast<InputActionType>(type) == InputActionType::Axis1D) {
                map->BindAxis1D(name, keys[2], keys[3], keys[4], keys[5]);
            } else {
                map->BindButton(name, keys[0], keys[1]);
            }
            while (*cursor != '\0' && *cursor != '"') {
                ++cursor;
            }
            if (*cursor == '"') {
                ++cursor;
            }
        }
        return true;
    }
};

class PlayerInputSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::PlayerInput; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "player_input"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        if (owner.GetComponent<PlayerInputComponent>() == nullptr) {
            return false;
        }
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String("1");
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        PlayerInputComponent* input = owner.GetComponent<PlayerInputComponent>();
        if (input == nullptr) {
            input = owner.AddComponent<PlayerInputComponent>();
        }
        if (InputActionMapComponent* map = owner.GetComponent<InputActionMapComponent>()) {
            input->SetActionMap(map);
        }
        return true;
    }
};

class TilemapObjectSpawnSnapshotHandler final : public IComponentSnapshotHandler {
public:
    [[nodiscard]] ComponentKind GetKind() const noexcept override { return ComponentKind::TilemapObjectSpawn; }
    [[nodiscard]] const char* GetKindTag() const noexcept override { return "tilemap_object_spawn"; }

    [[nodiscard]] bool TryCapture(
            const GameObject& owner,
            const SceneCaptureContext& /*ctx*/,
            ComponentRecord& out) const override {
        const TilemapObjectSpawnComponent* spawn = owner.GetComponent<TilemapObjectSpawnComponent>();
        if (spawn == nullptr) {
            return false;
        }
        char buf[32]{};
        std::snprintf(buf, sizeof(buf), "%d", spawn->GetSpawnOnAttach() ? 1 : 0);
        out.kind = Utf8String(GetKindTag());
        out.payload = Utf8String(buf);
        return true;
    }

    [[nodiscard]] bool TryRestore(
            GameObject& owner,
            const ComponentRecord& record,
            GameWorld& /*world*/,
            const SceneApplyContext& /*ctx*/) const override {
        if (!KindTagEquals(record.kind, GetKindTag())) {
            return false;
        }
        int onAttach = 1;
        std::sscanf(record.payload.CStr(), "%d", &onAttach);
        TilemapObjectSpawnComponent* spawn = owner.GetComponent<TilemapObjectSpawnComponent>();
        if (spawn == nullptr) {
            spawn = owner.AddComponent<TilemapObjectSpawnComponent>();
        }
        spawn->SetSpawnOnAttach(onAttach != 0);
        return true;
    }
};

}  // namespace

void RegisterGameFlowSnapshotHandlers(ComponentSnapshotRegistry& registry) {
    RegisterHandler<GameStateSnapshotHandler>(registry);
    RegisterHandler<GameFlowTriggerSnapshotHandler>(registry);
    RegisterHandler<InputActionMapSnapshotHandler>(registry);
    RegisterHandler<PlayerInputSnapshotHandler>(registry);
    RegisterHandler<TilemapObjectSpawnSnapshotHandler>(registry);
}

}  // namespace Spark
