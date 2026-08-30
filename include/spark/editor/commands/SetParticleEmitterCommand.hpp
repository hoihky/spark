#pragma once

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/math/Vector4.hpp"

namespace Spark {

class GameObject;

namespace Editor {

struct ParticleEmitterState {
    bool enabled = true;
    float emissionRate = 48.0F;
    float lifeMin = 0.8F;
    float lifeMax = 1.6F;
    float sizeStart = 0.14F;
    float sizeEnd = 0.02F;
    Vector4 colorStart{0.95F, 0.85F, 0.35F, 1.0F};
    Vector4 colorEnd{0.9F, 0.2F, 0.05F, 0.0F};
    float spreadRadians = 0.55F;
    float speedMin = 1.2F;
    float speedMax = 2.8F;
    bool useLocalEmission = false;
    int emissionModule = 0;
    float ringRadius = 0.35F;
};

/** Undoable edit of <c>ParticleEmitterComponent</c> inspector fields. */
class SetParticleEmitterCommand final : public IEditorCommand {
public:
    SetParticleEmitterCommand(GameObject& target, ParticleEmitterState before, ParticleEmitterState after);

    void Undo() override;
    void Redo() override;
    [[nodiscard]] Utf8String GetDescription() const override;

    [[nodiscard]] static bool NearlyEqual(const ParticleEmitterState& a, const ParticleEmitterState& b) noexcept;
    [[nodiscard]] static ParticleEmitterState Capture(GameObject& target);

private:
    static void Apply(GameObject& target, const ParticleEmitterState& state);
    static const char* ModuleIdFromIndex(int index) noexcept;
    static int ModuleIndexFromId(const char* moduleId) noexcept;

    GameObject* target = nullptr;
    ParticleEmitterState before{};
    ParticleEmitterState after{};
};

}  // namespace Editor
}  // namespace Spark
