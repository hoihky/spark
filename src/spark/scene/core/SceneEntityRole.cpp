#include "spark/scene/core/SceneEntityRole.hpp"

#include "spark/scene/core/GameWorld.hpp"

#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

bool KindEquals(const Utf8String& kind, const char* tag) noexcept {
    return tag != nullptr && std::strcmp(kind.CStr(), tag) == 0;
}

}  // namespace

SceneEntityRoleInfo SceneEntityRoleClassifier::Classify(const EntityRecord& entity) noexcept {
    SceneEntityRoleInfo info{};
    for (std::size_t ci = 0; ci < entity.components.GetSize(); ++ci) {
        const ComponentRecord& component = entity.components[ci];
        if (KindEquals(component.kind, "mesh")) {
            info.role = SceneEntityRole::Mesh;
            char slotTag[32]{};
            char asset[384]{};
            float albedoR = 1.0F;
            float albedoG = 1.0F;
            float albedoB = 1.0F;
            if (std::sscanf(
                        component.payload.CStr(),
                        "%31s \"%383[^\"]\" %f %f %f",
                        slotTag,
                        asset,
                        &albedoR,
                        &albedoG,
                        &albedoB)
                >= 2) {
                info.meshAssetPath = Utf8String(asset);
            }
        } else if (KindEquals(component.kind, "gltf_scene")) {
            info.role = SceneEntityRole::Mesh;
            const char* cursor = component.payload.CStr();
            char asset[384]{};
            while (*cursor == ' ' || *cursor == '\t') {
                ++cursor;
            }
            if (*cursor == '"') {
                ++cursor;
                std::size_t n = 0;
                while (*cursor != '\0' && *cursor != '"' && n + 1 < sizeof(asset)) {
                    asset[n++] = *cursor++;
                }
                asset[n] = '\0';
                if (n > 0) {
                    info.meshAssetPath = Utf8String(asset);
                }
            }
        } else if (KindEquals(component.kind, "point_light") || KindEquals(component.kind, "spot_light")) {
            info.role = SceneEntityRole::Light;
        } else if (KindEquals(component.kind, "spawn_point")) {
            info.role = SceneEntityRole::SpawnPoint;
        }
    }
    return info;
}

void SceneEntityRoleClassifier::SortObjectsById(Array<GameObject*>& objects) noexcept {
    for (std::size_t i = 1; i < objects.GetSize(); ++i) {
        GameObject* key = objects[i];
        const std::uint64_t keyId = key != nullptr ? key->GetId() : 0;
        std::size_t j = i;
        while (j > 0) {
            GameObject* previous = objects[j - 1];
            const std::uint64_t previousId = previous != nullptr ? previous->GetId() : 0;
            if (previousId <= keyId) {
                break;
            }
            objects[j] = objects[j - 1];
            --j;
        }
        objects[j] = key;
    }
}

Array<GameObject*> SceneEntityRoleClassifier::CollectInstanceObjects(
        const GameWorld& world,
        const SceneInstanceId instanceId) noexcept {
    Array<GameObject*> objects{};
    world.ForEachGameObject([&](GameObject* object) {
        if (object != nullptr && object->GetSceneInstanceId() == instanceId) {
            objects.PushBack(object);
        }
    });
    SortObjectsById(objects);
    return objects;
}

}  // namespace Spark
