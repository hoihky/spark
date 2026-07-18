#include "spark/scene/RenderVolumes.hpp"

#include "spark/ecs/components/rendering/FogVolumeComponent.hpp"
#include "spark/ecs/components/rendering/PostProcessVolumeComponent.hpp"
#include "spark/ecs/components/world/TimeOfDayDriverComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/VolumeRegions.hpp"

namespace Spark {

namespace {

FrameTimeOfDayState gTimeOfDay{};

}  // namespace

FrameTimeOfDayState& GetFrameTimeOfDayState() noexcept {
    return gTimeOfDay;
}

const FrameTimeOfDayState& GetFrameTimeOfDayStateConst() noexcept {
    return gTimeOfDay;
}

void ProcessTimeOfDayDrivers(GameWorld& world, const float deltaTimeSeconds) noexcept {
    gTimeOfDay = FrameTimeOfDayState{};
    bool found = false;
    std::int32_t bestPriority = 0;
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        TimeOfDayDriverComponent* driver = o->GetComponent<TimeOfDayDriverComponent>();
        if (driver == nullptr || !driver->IsEnabled()) {
            return;
        }
        if (driver->IsLooping() && driver->GetDayLengthSeconds() > 1.0e-4F) {
            float t = driver->GetTimeOfDay() + deltaTimeSeconds / driver->GetDayLengthSeconds();
            while (t >= 1.0F) {
                t -= 1.0F;
            }
            driver->SetTimeOfDay(t);
        }
        if (!found || driver->GetPriority() > bestPriority) {
            found = true;
            bestPriority = driver->GetPriority();
            gTimeOfDay.active = true;
            gTimeOfDay.useTimeOfDay = true;
            gTimeOfDay.timeOfDay = driver->GetTimeOfDay();
        }
    });
}

void ApplyRegionalRenderVolumes(
        const GameWorld& world,
        const Vector3& cameraPositionWorld,
        SceneRenderParams& params) noexcept {
    if (gTimeOfDay.active) {
        params.useTimeOfDay = gTimeOfDay.useTimeOfDay;
        params.timeOfDay = gTimeOfDay.timeOfDay;
    }

    bool fogFound = false;
    std::int32_t fogPriority = 0;
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const FogVolumeComponent* fog = o->GetComponent<FogVolumeComponent>();
        if (fog == nullptr || !fog->IsEnabled()) {
            return;
        }
        if (!PointInsideVolume(cameraPositionWorld, *o, fog->GetShape(), fog->GetHalfExtents())) {
            return;
        }
        if (!fogFound || fog->GetPriority() > fogPriority) {
            fogFound = true;
            fogPriority = fog->GetPriority();
            params.fogEnabled = true;
            params.fogColor = fog->GetFogColor();
            params.fogDensity = fog->GetFogDensity();
            params.fogStart = fog->GetFogStart();
            params.fogEnd = fog->GetFogEnd();
        }
    });

    bool postFound = false;
    std::int32_t postPriority = 0;
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        const PostProcessVolumeComponent* post = o->GetComponent<PostProcessVolumeComponent>();
        if (post == nullptr || !post->IsEnabled()) {
            return;
        }
        if (!PointInsideVolume(cameraPositionWorld, *o, post->GetShape(), post->GetHalfExtents())) {
            return;
        }
        if (!postFound || post->GetPriority() > postPriority) {
            postFound = true;
            postPriority = post->GetPriority();
            if (post->HasSsaoOverride()) {
                params.ssaoEnabled = post->GetSsaoEnabled();
            }
            if (post->HasExposureOverride()) {
                params.exposure = post->GetExposure();
            }
            if (post->HasAmbientScaleOverride()) {
                params.ambientScale = post->GetAmbientScale();
            }
        }
    });
}

}  // namespace Spark
