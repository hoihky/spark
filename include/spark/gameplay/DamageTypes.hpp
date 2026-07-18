#pragma once

#include <cstdint>

namespace Spark {

class GameObject;

/** Payload for <c>SignalId::DamageApplied</c> (stack-local during dispatch). */
struct DamageSignalPayload {
    float amount = 0.0F;
    float applied = 0.0F;
    GameObject* instigator = nullptr;
    GameObject* target = nullptr;
};

}  // namespace Spark
