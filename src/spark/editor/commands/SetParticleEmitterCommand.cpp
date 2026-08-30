#include "spark/editor/commands/SetParticleEmitterCommand.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/math/Constants.hpp"
#include "spark/scene/vfx/modules/ParticleModuleRegistry.hpp"

#include <cmath>
#include <cstring>

namespace Spark::Editor {

namespace {

bool FloatNearlyEqual(const float a, const float b) noexcept {
    return std::fabs(a - b) <= Epsilon;
}

bool Vector4NearlyEqual(const Vector4& a, const Vector4& b) noexcept {
    return FloatNearlyEqual(a.x, b.x) && FloatNearlyEqual(a.y, b.y) && FloatNearlyEqual(a.z, b.z) &&
           FloatNearlyEqual(a.w, b.w);
}

}  // namespace

SetParticleEmitterCommand::SetParticleEmitterCommand(
        GameObject& target,
        ParticleEmitterState before,
        ParticleEmitterState after)
    : target(&target), before(MoveTemp(before)), after(MoveTemp(after)) {}

const char* SetParticleEmitterCommand::ModuleIdFromIndex(const int index) noexcept {
    switch (index) {
        case 1:
            return "burst_only";
        case 2:
            return "ring";
        default:
            return ParticleModuleRegistry::DefaultModuleId();
    }
}

int SetParticleEmitterCommand::ModuleIndexFromId(const char* const moduleId) noexcept {
    if (moduleId == nullptr) {
        return 0;
    }
    if (std::strcmp(moduleId, "burst_only") == 0) {
        return 1;
    }
    if (std::strcmp(moduleId, "ring") == 0) {
        return 2;
    }
    return 0;
}

void SetParticleEmitterCommand::Apply(GameObject& target, const ParticleEmitterState& state) {
    if (ParticleEmitterComponent* emitter = target.GetComponent<ParticleEmitterComponent>()) {
        emitter->SetEmitterEnabled(state.enabled);
        emitter->SetEmissionRate(state.emissionRate);
        emitter->SetLifetime(state.lifeMin, state.lifeMax);
        emitter->SetStartEndSize(state.sizeStart, state.sizeEnd);
        emitter->SetStartEndColor(state.colorStart, state.colorEnd);
        emitter->SetSpreadAngleRadians(state.spreadRadians);
        emitter->SetSpeedRange(state.speedMin, state.speedMax);
        emitter->SetUseLocalEmission(state.useLocalEmission);
        emitter->SetEmissionModuleId(ModuleIdFromIndex(state.emissionModule));
        emitter->SetRingRadius(state.ringRadius);
    }
}

void SetParticleEmitterCommand::Undo() {
    if (target != nullptr) {
        Apply(*target, before);
    }
}

void SetParticleEmitterCommand::Redo() {
    if (target != nullptr) {
        Apply(*target, after);
    }
}

Utf8String SetParticleEmitterCommand::GetDescription() const {
    if (target != nullptr) {
        Utf8String desc = Utf8String("Particle Emitter ");
        desc.AppendUtf8(target->GetName().CStr());
        return desc;
    }
    return Utf8String("Particle emitter edit");
}

bool SetParticleEmitterCommand::NearlyEqual(const ParticleEmitterState& a, const ParticleEmitterState& b) noexcept {
    return a.enabled == b.enabled && FloatNearlyEqual(a.emissionRate, b.emissionRate) &&
           FloatNearlyEqual(a.lifeMin, b.lifeMin) && FloatNearlyEqual(a.lifeMax, b.lifeMax) &&
           FloatNearlyEqual(a.sizeStart, b.sizeStart) && FloatNearlyEqual(a.sizeEnd, b.sizeEnd) &&
           Vector4NearlyEqual(a.colorStart, b.colorStart) && Vector4NearlyEqual(a.colorEnd, b.colorEnd) &&
           FloatNearlyEqual(a.spreadRadians, b.spreadRadians) && FloatNearlyEqual(a.speedMin, b.speedMin) &&
           FloatNearlyEqual(a.speedMax, b.speedMax) && a.useLocalEmission == b.useLocalEmission &&
           a.emissionModule == b.emissionModule && FloatNearlyEqual(a.ringRadius, b.ringRadius);
}

ParticleEmitterState SetParticleEmitterCommand::Capture(GameObject& target) {
    ParticleEmitterState state{};
    if (const ParticleEmitterComponent* emitter = target.GetComponent<ParticleEmitterComponent>()) {
        state.enabled = emitter->IsEmitterEnabled();
        state.emissionRate = emitter->GetEmissionRate();
        state.lifeMin = emitter->GetLifetimeMin();
        state.lifeMax = emitter->GetLifetimeMax();
        state.sizeStart = emitter->GetStartSize();
        state.sizeEnd = emitter->GetEndSize();
        state.colorStart = emitter->GetColorStart();
        state.colorEnd = emitter->GetColorEnd();
        state.spreadRadians = emitter->GetSpreadAngleRadians();
        state.speedMin = emitter->GetSpeedMin();
        state.speedMax = emitter->GetSpeedMax();
        state.useLocalEmission = emitter->GetUseLocalEmission();
        state.emissionModule = ModuleIndexFromId(emitter->GetEmissionModuleId());
        state.ringRadius = emitter->GetRingRadius();
    }
    return state;
}

}  // namespace Spark::Editor
