#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;
class GameWorld;

struct VfxPlayRequest {
    Utf8String assetKeyOrBuiltin;
    Vector3 worldPosition{};
};

/**
 * Queues world-space one-shots and recycles hidden pooled effect entities.
 * Call <c>ProcessVfx</c> each frame after component simulation.
 */
class VfxSubsystem {
public:
    void Queue(VfxPlayRequest request);
    void Queue(const char* assetKeyOrBuiltin, const Vector3& worldPosition);

    void Process(GameWorld& world);

    void Reset() noexcept;
    /** Destroys pooled effect entities still owned by the subsystem. */
    void Shutdown(GameWorld& world) noexcept;

private:
    [[nodiscard]] GameObject* Acquire(GameWorld& world);
    void Release(GameWorld& world, GameObject* object);

    Array<VfxPlayRequest> pending{};
    Array<GameObject*> pool{};
    Array<GameObject*> active{};
};

}  // namespace Spark
