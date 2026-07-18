#include "spark/audio/AudioSpatial.hpp"

#include "spark/audio/AudioListenerPose.hpp"
#include "spark/ecs/components/audio/AudioListenerComponent.hpp"
#include "spark/ecs/components/camera/CameraComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/Camera.hpp"
#include "spark/scene/GameWorld.hpp"

#include <cmath>

namespace Spark {

namespace {

AudioListenerPose gFrameListener{};

[[nodiscard]] Vector3 NormalizeOr(const Vector3& v, const Vector3& fallback) noexcept {
    const float len2 = v.LengthSquared();
    if (len2 <= 1.0e-12F) {
        return fallback;
    }
    return v * (1.0F / std::sqrt(len2));
}

}  // namespace

AudioListenerPose& GetFrameAudioListenerPose() noexcept {
    return gFrameListener;
}

const AudioListenerPose& GetFrameAudioListenerPoseConst() noexcept {
    return gFrameListener;
}

void ProcessAudioListeners(const GameWorld& world) noexcept {
    gFrameListener.valid = false;
    bool found = false;
    std::int32_t bestPriority = 0;
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const AudioListenerComponent* listener = o->GetComponent<AudioListenerComponent>();
        if (listener == nullptr || !listener->IsEnabled()) {
            return;
        }
        if (!found || listener->GetPriority() > bestPriority) {
            found = true;
            bestPriority = listener->GetPriority();
            const Matrix4& worldM = o->GetWorldMatrix();
            gFrameListener.position = {worldM.m[12], worldM.m[13], worldM.m[14]};
            gFrameListener.right = NormalizeOr({worldM.m[0], worldM.m[1], worldM.m[2]}, Vector3::UnitX);
            gFrameListener.up = NormalizeOr({worldM.m[4], worldM.m[5], worldM.m[6]}, Vector3::UnitY);
            gFrameListener.forward = NormalizeOr(
                    {-worldM.m[8], -worldM.m[9], -worldM.m[10]},
                    Vector3{0.0F, 0.0F, -1.0F});
            gFrameListener.valid = true;
        }
    });

    if (gFrameListener.valid) {
        return;
    }

    // Fallback: main 3D camera pose when no explicit listener is present.
    ResolvedCamera resolved{};
    if (!TryResolveMainCamera(world, resolved) || resolved.object == nullptr ||
        resolved.component == nullptr) {
        return;
    }
    const CameraComponent* cam = resolved.component;
    gFrameListener.position = cam->WorldPosition(*resolved.object);
    cam->BillboardBasisWorld(*resolved.object, gFrameListener.right, gFrameListener.up);
    gFrameListener.forward = NormalizeOr(
            Vector3::Cross(gFrameListener.right, gFrameListener.up),
            Vector3{0.0F, 0.0F, -1.0F});
    gFrameListener.valid = true;
}

}  // namespace Spark
