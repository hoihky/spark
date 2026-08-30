#include "spark/scene/vfx/modules/ParticleModuleRegistry.hpp"

#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Constants.hpp"

#include <cmath>
#include <cstring>

namespace Spark {

namespace {

class ContinuousEmissionModule final : public IParticleModule {
public:
    [[nodiscard]] const char* GetId() const noexcept override { return "continuous"; }

    void EmitFrame(
            ParticleEmitterComponent& emitter,
            GameObject& /*owner*/,
            const Vector3& origin,
            const Vector3& worldEmissionDir,
            const float deltaTimeSeconds) const override {
        emitter.EmitContinuous(origin, worldEmissionDir, deltaTimeSeconds);
    }
};

class BurstOnlyEmissionModule final : public IParticleModule {
public:
    [[nodiscard]] const char* GetId() const noexcept override { return "burst_only"; }

    void EmitFrame(
            ParticleEmitterComponent& /*emitter*/,
            GameObject& /*owner*/,
            const Vector3& /*origin*/,
            const Vector3& /*worldEmissionDir*/,
            float /*deltaTimeSeconds*/) const override {}
};

class RingEmissionModule final : public IParticleModule {
public:
    [[nodiscard]] const char* GetId() const noexcept override { return "ring"; }

    void EmitFrame(
            ParticleEmitterComponent& emitter,
            GameObject& /*owner*/,
            const Vector3& origin,
            const Vector3& worldEmissionDir,
            const float deltaTimeSeconds) const override {
        emitter.EmitRing(origin, worldEmissionDir, deltaTimeSeconds);
    }
};

const ContinuousEmissionModule kContinuous{};
const BurstOnlyEmissionModule kBurstOnly{};
const RingEmissionModule kRing{};

}  // namespace

const IParticleModule& ParticleModuleRegistry::Get(const char* const moduleId) noexcept {
    if (moduleId == nullptr || moduleId[0] == '\0') {
        return kContinuous;
    }
    if (std::strcmp(moduleId, "burst_only") == 0) {
        return kBurstOnly;
    }
    if (std::strcmp(moduleId, "ring") == 0) {
        return kRing;
    }
    return kContinuous;
}

void ParticleModuleRegistry::EmitFrame(
        ParticleEmitterComponent& emitter,
        GameObject& owner,
        const Vector3& origin,
        const Vector3& worldEmissionDir,
        const float deltaTimeSeconds) noexcept {
    Get(emitter.GetEmissionModuleId()).EmitFrame(emitter, owner, origin, worldEmissionDir, deltaTimeSeconds);
}

}  // namespace Spark
