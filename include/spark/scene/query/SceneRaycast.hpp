#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;

/** World-space pick ray (direction should be normalized). */
struct SceneRay {
    Vector3 origin{Vector3::Zero};
    Vector3 direction{Vector3::UnitZ};
    float tMin = 1.0e-4F;
    float tMax = 1.0e30F;
};

/** Result of <c>Scene::RaycastPick</c>. */
struct SceneRaycastHit {
    GameObject* object = nullptr;
    float distance = 0.0F;
    Vector3 pointWorld{Vector3::Zero};
    Vector3 normalWorld{Vector3::Zero};
};

struct SceneRaycastOptions {
    bool pickRigidMeshes = true;
    bool pickSkinnedMeshes = true;
    /** When false (default), inactive hierarchy nodes are not pickable. */
    bool includeInactive = false;
};

}  // namespace Spark
