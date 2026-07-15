#include "spark/scripting/SparkInterop.h"
#include "spark/scripting/SparkInteropInternal.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/AnimatorComponent.hpp"
#include "spark/ecs/components/SpriteAnimatorComponent.hpp"
#include "spark/ecs/components/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/CollisionComponent.hpp"
#include "spark/ecs/components/GuiCanvasComponent.hpp"
#include "spark/ecs/components/MaterialComponent.hpp"
#include "spark/ecs/components/MeshComponent.hpp"
#include "spark/ecs/components/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/PointLightComponent.hpp"
#include "spark/ecs/components/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/SkyComponent.hpp"
#include "spark/ecs/components/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/SpotLightComponent.hpp"
#include "spark/ecs/components/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/Sprite2DCharacterAnimFsmComponent.hpp"
#include "spark/ecs/components/Camera2DComponent.hpp"
#include "spark/ecs/components/Camera2DRigComponent.hpp"
#include "spark/ecs/components/RenderLayerComponent.hpp"
#include "spark/ecs/components/SortingGroupComponent.hpp"
#include "spark/ecs/components/SpriteComponent.hpp"
#include "spark/ecs/components/TextOverlayComponent.hpp"
#include "spark/ecs/components/TilemapComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/scene/RenderLayerRegistry.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GameWorld.hpp"

#include <cstring>

using namespace Spark::Scripting;

namespace {

Spark::SceneMeshSlot ToCppMeshSlot(const SparkSceneMeshSlot slot) {
    return static_cast<Spark::SceneMeshSlot>(static_cast<std::uint8_t>(slot));
}

Spark::SceneSkyMode ToCppSkyMode(const SparkSceneSkyMode mode) {
    return static_cast<Spark::SceneSkyMode>(static_cast<std::uint8_t>(mode));
}

Spark::GameObject* ToObject(SparkGameObject* object) {
    return reinterpret_cast<Spark::GameObject*>(object);
}

Spark::GameWorld* ToWorld(SparkGameWorld* world) {
    return reinterpret_cast<Spark::GameWorld*>(world);
}

}  // namespace

extern "C" {

int spark_object_get_name(const SparkGameObject* object, char* outUtf8, const std::uint32_t outCapacity) {
    if (object == nullptr || outUtf8 == nullptr || outCapacity == 0) {
        return 0;
    }
    const char* name = ToObject(const_cast<SparkGameObject*>(object))->GetName().CStr();
    if (name == nullptr) {
        outUtf8[0] = '\0';
        return 0;
    }
    const std::size_t len = std::strlen(name);
    const std::size_t copy = (len < outCapacity - 1) ? len : (outCapacity - 1);
    std::memcpy(outUtf8, name, copy);
    outUtf8[copy] = '\0';
    return static_cast<int>(copy);
}

void spark_object_set_name(SparkGameObject* object, const char* utf8Name) {
    if (object == nullptr) {
        return;
    }
    ToObject(object)->GetName() = utf8Name != nullptr ? Spark::Utf8String(utf8Name) : Spark::Utf8String();
}

SparkGameComponent* spark_object_get_or_add_transform(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    Spark::GameObject* o = ToObject(object);
    if (Spark::TransformComponent* existing = o->GetComponent<Spark::TransformComponent>()) {
        return reinterpret_cast<SparkGameComponent*>(existing);
    }
    return reinterpret_cast<SparkGameComponent*>(o->AddComponent<Spark::TransformComponent>());
}

SparkGameComponent* spark_object_add_mesh(
        SparkGameObject* object,
        const SparkSceneMeshSlot slot,
        const SparkVector3* albedo) {
    if (object == nullptr || albedo == nullptr) {
        return nullptr;
    }
    Spark::SharedPtr<Spark::Mesh> empty{};
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::MeshComponent>(
            empty, ToCppMeshSlot(slot), ToVector3(*albedo)));
}

SparkGameComponent* spark_object_add_material_lit(
        SparkGameObject* object,
        const char* textureKeyOrPath,
        const SparkVector3* tint) {
    if (object == nullptr || tint == nullptr) {
        return nullptr;
    }
    Spark::SharedPtr<Spark::Texture2D> tex{};
    if (textureKeyOrPath != nullptr) {
        tex = ToObject(object)->GetWorld().TryGetTextureByKeyOrPath(textureKeyOrPath);
    }
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::MaterialComponent>(tex, ToVector3(*tint)));
}

SparkGameComponent* spark_object_add_material_emissive(
        SparkGameObject* object,
        const SparkVector3* emissiveRgb,
        const float emissiveIntensity) {
    if (object == nullptr || emissiveRgb == nullptr) {
        return nullptr;
    }
    auto* mat = ToObject(object)->AddComponent<Spark::MaterialComponent>();
    mat->SetEmissive(ToVector3(*emissiveRgb), emissiveIntensity);
    return reinterpret_cast<SparkGameComponent*>(mat);
}

SparkGameComponent* spark_object_add_sprite(
        SparkGameObject* object,
        const char* textureKeyOrPath,
        const SparkVector4* tint,
        const SparkVector4* uvRect,
        const int sortOrder) {
    if (object == nullptr || tint == nullptr || uvRect == nullptr) {
        return nullptr;
    }
    Spark::SharedPtr<Spark::Texture2D> tex{};
    if (textureKeyOrPath != nullptr) {
        tex = ToObject(object)->GetWorld().TryGetTextureByKeyOrPath(textureKeyOrPath);
    }
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::SpriteComponent>(
            tex, ToVector4(*tint), ToVector4(*uvRect), sortOrder));
}

SparkGameComponent* spark_object_add_sprite_animator(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::SpriteAnimatorComponent>());
}

SparkGameComponent* spark_object_add_sprite_2d_character_anim_fsm(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::Sprite2DCharacterAnimFsmComponent>());
}

SparkGameComponent* spark_object_add_character_3d_anim_fsm(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::Character3DAnimFsmComponent>());
}

SparkGameComponent* spark_object_add_tilemap(
        SparkGameObject* object,
        const char* atlasKeyOrPath,
        const std::uint32_t mapWidth,
        const std::uint32_t mapHeight,
        const std::uint32_t atlasTilesU,
        const std::uint32_t atlasTilesV,
        const float tileWorldSize,
        const int sortOrderBase) {
    if (object == nullptr) {
        return nullptr;
    }
    Spark::SharedPtr<Spark::Texture2D> atlas{};
    if (atlasKeyOrPath != nullptr) {
        atlas = ToObject(object)->GetWorld().TryGetTextureByKeyOrPath(atlasKeyOrPath);
    }
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::TilemapComponent>(
            atlas, mapWidth, mapHeight, atlasTilesU, atlasTilesV, tileWorldSize, sortOrderBase));
}

SparkGameComponent* spark_object_add_point_light(
        SparkGameObject* object,
        const SparkVector3* color,
        const float intensity,
        const float range) {
    if (object == nullptr || color == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::PointLightComponent>(
            ToVector3(*color), intensity, range));
}

SparkGameComponent* spark_object_add_spot_light(
        SparkGameObject* object,
        const SparkVector3* color,
        const float intensity,
        const float range,
        const float innerConeDegrees,
        const float outerConeDegrees) {
    if (object == nullptr || color == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::SpotLightComponent>(
            ToVector3(*color), intensity, range, innerConeDegrees, outerConeDegrees));
}

SparkGameComponent* spark_object_add_sky(SparkGameObject* object, const SparkSceneSkyMode mode) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::SkyComponent>(ToCppSkyMode(mode)));
}

SparkGameComponent* spark_object_add_text_overlay(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::TextOverlayComponent>());
}

SparkGameComponent* spark_object_add_gui_canvas(SparkGameObject* object, const int sortOrder) {
    if (object == nullptr) {
        return nullptr;
    }
    auto* canvas = ToObject(object)->AddComponent<Spark::GuiCanvasComponent>();
    canvas->SetSortOrder(sortOrder);
    return reinterpret_cast<SparkGameComponent*>(canvas);
}

SparkGameComponent* spark_object_add_particle_emitter(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::ParticleEmitterComponent>());
}

SparkGameComponent* spark_object_add_box_collider_2d(
        SparkGameObject* object,
        const SparkVector2* halfExtents,
        const SparkVector2* offset) {
    if (object == nullptr || halfExtents == nullptr) {
        return nullptr;
    }
    const Spark::Vector2 off = offset != nullptr ? ToVector2(*offset) : Spark::Vector2::Zero;
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::BoxCollider2DComponent>(
            ToVector2(*halfExtents), off));
}

SparkGameComponent* spark_object_add_circle_collider_2d(
        SparkGameObject* object,
        const float radius,
        const SparkVector2* offset) {
    if (object == nullptr) {
        return nullptr;
    }
    const Spark::Vector2 off = offset != nullptr ? ToVector2(*offset) : Spark::Vector2::Zero;
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::CircleCollider2DComponent>(radius, off));
}

SparkGameComponent* spark_object_add_rigidbody_2d(
        SparkGameObject* object,
        const SparkRigidbodyBodyType2D bodyType,
        const float gravityScale) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::Rigidbody2DComponent>(
            static_cast<Spark::RigidbodyBodyType2D>(static_cast<std::uint8_t>(bodyType)), gravityScale));
}

SparkGameComponent* spark_object_add_box_collider_3d(
        SparkGameObject* object,
        const SparkVector3* halfExtents,
        const SparkVector3* offset) {
    if (object == nullptr || halfExtents == nullptr) {
        return nullptr;
    }
    const Spark::Vector3 off = offset != nullptr ? ToVector3(*offset) : Spark::Vector3::Zero;
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::BoxCollider3DComponent>(
            ToVector3(*halfExtents), off));
}

SparkGameComponent* spark_object_add_sphere_collider_3d(
        SparkGameObject* object,
        const float radius,
        const SparkVector3* offset) {
    if (object == nullptr) {
        return nullptr;
    }
    const Spark::Vector3 off = offset != nullptr ? ToVector3(*offset) : Spark::Vector3::Zero;
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::SphereCollider3DComponent>(radius, off));
}

SparkGameComponent* spark_object_add_rigidbody_3d(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::Rigidbody3DComponent>());
}

SparkGameComponent* spark_object_add_collision_sphere(
        SparkGameObject* object,
        const float radius,
        const SparkVector3* localCenter) {
    if (object == nullptr) {
        return nullptr;
    }
    const Spark::Vector3 center = localCenter != nullptr ? ToVector3(*localCenter) : Spark::Vector3::Zero;
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::CollisionComponent>(radius, center));
}

int spark_object_add_skinned_character_from_gltf(
        SparkGameObject* object,
        SparkGameWorld* world,
        const char* gltfPath,
        SparkGameComponent** outSkinnedMesh,
        SparkGameComponent** outAnimator,
        SparkGameComponent** outMaterial) {
    if (object == nullptr || world == nullptr || gltfPath == nullptr) {
        return 0;
    }
    const Spark::SkinnedGltfAsset asset = ToWorld(world)->LoadSkinnedGltf(gltfPath);
    if (!asset.mesh) {
        return 0;
    }
    Spark::GameObject* o = ToObject(object);
    auto* fsm = o->AddComponent<Spark::Character3DAnimFsmComponent>();
    auto* smc = o->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);
    auto* anim = o->AddComponent<Spark::AnimatorComponent>(asset.skeleton, asset.walkClipIndex, 1.0F);
    fsm->ConfigureLocomotionFromSkeleton(*asset.skeleton, asset.walkClipIndex);
    auto* mat = o->AddComponent<Spark::MaterialComponent>(asset.baseColorTexture, Spark::Vector3::One);
    if (Spark::TransformComponent* tr = o->GetComponent<Spark::TransformComponent>()) {
        tr->SetRotation(asset.bindUpAlignment);
    }
    if (outSkinnedMesh != nullptr) {
        *outSkinnedMesh = reinterpret_cast<SparkGameComponent*>(smc);
    }
    if (outAnimator != nullptr) {
        *outAnimator = reinterpret_cast<SparkGameComponent*>(anim);
    }
    if (outMaterial != nullptr) {
        *outMaterial = reinterpret_cast<SparkGameComponent*>(mat);
    }
    return 1;
}

void spark_transform_get_translation(const SparkGameComponent* transform, SparkVector3* out) {
    const auto* tr = AsComponent<const Spark::TransformComponent>(transform, Spark::ComponentKind::Transform);
    if (tr == nullptr || out == nullptr) {
        return;
    }
    *out = FromVector3(tr->GetLocalTransform().translation);
}

void spark_transform_set_translation(SparkGameComponent* transform, const SparkVector3* v) {
    auto* tr = AsComponent<Spark::TransformComponent>(transform, Spark::ComponentKind::Transform);
    if (tr == nullptr || v == nullptr) {
        return;
    }
    tr->SetTranslation(ToVector3(*v));
}

void spark_transform_set_rotation(SparkGameComponent* transform, const SparkQuaternion* q) {
    auto* tr = AsComponent<Spark::TransformComponent>(transform, Spark::ComponentKind::Transform);
    if (tr == nullptr || q == nullptr) {
        return;
    }
    tr->SetRotation(ToQuaternion(*q));
}

void spark_transform_set_scale(SparkGameComponent* transform, const SparkVector3* s) {
    auto* tr = AsComponent<Spark::TransformComponent>(transform, Spark::ComponentKind::Transform);
    if (tr == nullptr || s == nullptr) {
        return;
    }
    tr->SetScale(ToVector3(*s));
}

void spark_transform_set_uniform_scale(SparkGameComponent* transform, const float s) {
    auto* tr = AsComponent<Spark::TransformComponent>(transform, Spark::ComponentKind::Transform);
    if (tr == nullptr) {
        return;
    }
    tr->SetUniformScale(s);
}

void spark_mesh_get_albedo(const SparkGameComponent* mesh, SparkVector3* out) {
    const auto* mc = AsComponent<const Spark::MeshComponent>(mesh, Spark::ComponentKind::Mesh);
    if (mc == nullptr || out == nullptr) {
        return;
    }
    *out = FromVector3(mc->GetAlbedo());
}

void spark_mesh_set_albedo(SparkGameComponent* mesh, const SparkVector3* albedo) {
    auto* mc = AsComponent<Spark::MeshComponent>(mesh, Spark::ComponentKind::Mesh);
    if (mc == nullptr || albedo == nullptr) {
        return;
    }
    mc->SetAlbedo(ToVector3(*albedo));
}

int spark_mesh_set_mesh_by_key(SparkGameComponent* mesh, SparkGameWorld* world, const char* meshKeyOrPath) {
    auto* mc = AsComponent<Spark::MeshComponent>(mesh, Spark::ComponentKind::Mesh);
    if (mc == nullptr || world == nullptr || meshKeyOrPath == nullptr) {
        return 0;
    }
    Spark::SharedPtr<Spark::Mesh> resolved = ToWorld(world)->TryGetMeshByKeyOrPath(meshKeyOrPath);
    if (!resolved) {
        return 0;
    }
    mc->SetMesh(resolved);
    return 1;
}

void spark_material_set_tint(SparkGameComponent* material, const SparkVector3* tint) {
    auto* mat = AsComponent<Spark::MaterialComponent>(material, Spark::ComponentKind::Material);
    if (mat == nullptr || tint == nullptr) {
        return;
    }
    mat->SetTint(ToVector3(*tint));
}

void spark_material_set_metallic(SparkGameComponent* material, const float metallic) {
    auto* mat = AsComponent<Spark::MaterialComponent>(material, Spark::ComponentKind::Material);
    if (mat != nullptr) {
        mat->SetMetallic(metallic);
    }
}

void spark_material_set_roughness(SparkGameComponent* material, const float roughness) {
    auto* mat = AsComponent<Spark::MaterialComponent>(material, Spark::ComponentKind::Material);
    if (mat != nullptr) {
        mat->SetRoughness(roughness);
    }
}

void spark_material_set_emissive(SparkGameComponent* material, const SparkVector3* rgb, const float intensity) {
    auto* mat = AsComponent<Spark::MaterialComponent>(material, Spark::ComponentKind::Material);
    if (mat == nullptr || rgb == nullptr) {
        return;
    }
    mat->SetEmissive(ToVector3(*rgb), intensity);
}

int spark_material_set_base_color_texture(
        SparkGameComponent* material,
        SparkGameWorld* world,
        const char* textureKeyOrPath) {
    auto* mat = AsComponent<Spark::MaterialComponent>(material, Spark::ComponentKind::Material);
    if (mat == nullptr || world == nullptr || textureKeyOrPath == nullptr) {
        return 0;
    }
    Spark::SharedPtr<Spark::Texture2D> tex = ToWorld(world)->TryGetTextureByKeyOrPath(textureKeyOrPath);
    if (!tex) {
        return 0;
    }
    mat->SetBaseColorTexture(tex);
    return 1;
}

void spark_sprite_set_tint(SparkGameComponent* sprite, const SparkVector4* tint) {
    auto* sc = AsComponent<Spark::SpriteComponent>(sprite, Spark::ComponentKind::Sprite);
    if (sc == nullptr || tint == nullptr) {
        return;
    }
    sc->SetTint(ToVector4(*tint));
}

void spark_sprite_set_uv_rect(SparkGameComponent* sprite, const SparkVector4* uvRect) {
    auto* sc = AsComponent<Spark::SpriteComponent>(sprite, Spark::ComponentKind::Sprite);
    if (sc == nullptr || uvRect == nullptr) {
        return;
    }
    sc->SetUvRect(ToVector4(*uvRect));
}

void spark_sprite_set_sort_order(SparkGameComponent* sprite, const int sortOrder) {
    auto* sc = AsComponent<Spark::SpriteComponent>(sprite, Spark::ComponentKind::Sprite);
    if (sc != nullptr) {
        sc->SetSortOrder(sortOrder);
    }
}

int spark_render_layer_register(const char* name, const int sortingOrder) {
    const Spark::RenderLayerId id =
            Spark::RenderLayerRegistry::Instance().RegisterLayer(name, static_cast<std::int16_t>(sortingOrder));
    return id == Spark::kInvalidRenderLayerId ? -1 : static_cast<int>(id);
}

int spark_render_layer_find(const char* name) {
    const Spark::RenderLayerId id = Spark::RenderLayerRegistry::Instance().FindLayerIdByName(name);
    return id == Spark::kInvalidRenderLayerId ? -1 : static_cast<int>(id);
}

SparkGameComponent* spark_object_add_render_layer(
        SparkGameObject* object,
        const char* layerName,
        const int orderInLayer) {
    if (object == nullptr || layerName == nullptr) {
        return nullptr;
    }
    Spark::RenderLayerRegistry& registry = Spark::RenderLayerRegistry::Instance();
    Spark::RenderLayerId layerId = registry.FindLayerIdByName(layerName);
    if (layerId == Spark::kInvalidRenderLayerId) {
        layerId = registry.RegisterLayer(layerName, 0);
    }
    if (layerId == Spark::kInvalidRenderLayerId) {
        return nullptr;
    }
    auto* layer = ToObject(object)->AddComponent<Spark::RenderLayerComponent>(layerId, orderInLayer);
    return reinterpret_cast<SparkGameComponent*>(layer);
}

void spark_render_layer_set_order_in_layer(SparkGameComponent* layer, const int orderInLayer) {
    auto* rl = AsComponent<Spark::RenderLayerComponent>(layer, Spark::ComponentKind::RenderLayer);
    if (rl != nullptr) {
        rl->SetOrderInLayer(orderInLayer);
    }
}

SparkGameComponent* spark_object_add_sorting_group(SparkGameObject* object, const int sortingOrder) {
    if (object == nullptr) {
        return nullptr;
    }
    auto* group = ToObject(object)->AddComponent<Spark::SortingGroupComponent>(sortingOrder);
    return reinterpret_cast<SparkGameComponent*>(group);
}

void spark_sorting_group_set_enabled(SparkGameComponent* group, const int enabled) {
    auto* sg = AsComponent<Spark::SortingGroupComponent>(group, Spark::ComponentKind::SortingGroup);
    if (sg != nullptr) {
        sg->SetEnabled(enabled != 0);
    }
}

void spark_sorting_group_set_sorting_order(SparkGameComponent* group, const int sortingOrder) {
    auto* sg = AsComponent<Spark::SortingGroupComponent>(group, Spark::ComponentKind::SortingGroup);
    if (sg != nullptr) {
        sg->SetSortingOrder(sortingOrder);
    }
}

void spark_sorting_group_set_sort_at_root_world_y(SparkGameComponent* group, const int enabled) {
    auto* sg = AsComponent<Spark::SortingGroupComponent>(group, Spark::ComponentKind::SortingGroup);
    if (sg != nullptr) {
        sg->SetSortAtRootWorldY(enabled != 0);
    }
}

SparkGameComponent* spark_object_add_camera_2d(
        SparkGameObject* object,
        const float halfExtentY,
        const int priority) {
    if (object == nullptr) {
        return nullptr;
    }
    auto* cam = ToObject(object)->AddComponent<Spark::Camera2DComponent>();
    cam->SetHalfExtentY(halfExtentY);
    cam->SetPriority(priority);
    return reinterpret_cast<SparkGameComponent*>(cam);
}

void spark_camera_2d_set_half_extent_y(SparkGameComponent* camera, const float halfExtentY) {
    auto* cam = AsComponent<Spark::Camera2DComponent>(camera, Spark::ComponentKind::Camera2D);
    if (cam != nullptr) {
        cam->SetHalfExtentY(halfExtentY);
    }
}

void spark_camera_2d_set_clip_planes(SparkGameComponent* camera, const float nearZ, const float farZ) {
    auto* cam = AsComponent<Spark::Camera2DComponent>(camera, Spark::ComponentKind::Camera2D);
    if (cam != nullptr) {
        cam->SetClipNearZ(nearZ);
        cam->SetClipFarZ(farZ);
    }
}

void spark_camera_2d_set_priority(SparkGameComponent* camera, const int priority) {
    auto* cam = AsComponent<Spark::Camera2DComponent>(camera, Spark::ComponentKind::Camera2D);
    if (cam != nullptr) {
        cam->SetPriority(priority);
    }
}

void spark_camera_2d_set_enabled(SparkGameComponent* camera, const int enabled) {
    auto* cam = AsComponent<Spark::Camera2DComponent>(camera, Spark::ComponentKind::Camera2D);
    if (cam != nullptr) {
        cam->SetEnabled(enabled != 0);
    }
}

SparkGameComponent* spark_object_add_camera_2d_rig(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(ToObject(object)->AddComponent<Spark::Camera2DRigComponent>());
}

void spark_camera_2d_rig_set_mode(SparkGameComponent* rig, const int mode) {
    auto* r = AsComponent<Spark::Camera2DRigComponent>(rig, Spark::ComponentKind::Camera2DRig);
    if (r != nullptr) {
        r->SetMode(static_cast<Spark::Camera2DRigMode>(static_cast<std::uint8_t>(mode)));
    }
}

void spark_camera_2d_rig_set_target(SparkGameComponent* rig, SparkGameObject* target) {
    auto* r = AsComponent<Spark::Camera2DRigComponent>(rig, Spark::ComponentKind::Camera2DRig);
    if (r != nullptr) {
        r->SetTarget(reinterpret_cast<Spark::GameObject*>(target));
    }
}

void spark_camera_2d_rig_set_target_offset(SparkGameComponent* rig, const SparkVector3* offset) {
    if (offset == nullptr) {
        return;
    }
    auto* r = AsComponent<Spark::Camera2DRigComponent>(rig, Spark::ComponentKind::Camera2DRig);
    if (r != nullptr) {
        r->SetTargetOffset(ToVector3(*offset));
    }
}

void spark_camera_2d_rig_set_follow_smooth_rate(SparkGameComponent* rig, const float rate) {
    auto* r = AsComponent<Spark::Camera2DRigComponent>(rig, Spark::ComponentKind::Camera2DRig);
    if (r != nullptr) {
        r->SetFollowSmoothRate(rate);
    }
}

void spark_camera_2d_rig_set_look_ahead_scale(SparkGameComponent* rig, const float scale) {
    auto* r = AsComponent<Spark::Camera2DRigComponent>(rig, Spark::ComponentKind::Camera2DRig);
    if (r != nullptr) {
        r->SetLookAheadScale(scale);
    }
}

void spark_camera_2d_rig_set_bounds(
        SparkGameComponent* rig,
        const int useBounds,
        const SparkVector2* boundsMin,
        const SparkVector2* boundsMax) {
    auto* r = AsComponent<Spark::Camera2DRigComponent>(rig, Spark::ComponentKind::Camera2DRig);
    if (r == nullptr) {
        return;
    }
    r->SetUseBounds(useBounds != 0);
    if (boundsMin != nullptr) {
        r->SetBoundsMin(ToVector2(*boundsMin));
    }
    if (boundsMax != nullptr) {
        r->SetBoundsMax(ToVector2(*boundsMax));
    }
}

void spark_camera_2d_rig_tick(
        SparkGameComponent* rig,
        SparkGameObject* owner,
        const float deltaSeconds,
        const float framebufferAspect) {
    auto* r = AsComponent<Spark::Camera2DRigComponent>(rig, Spark::ComponentKind::Camera2DRig);
    Spark::GameObject* ownerObj = ToObject(owner);
    if (r == nullptr || ownerObj == nullptr) {
        return;
    }
    Spark::Camera2DRigComponent::Tick(*r, *ownerObj, deltaSeconds, framebufferAspect);
}

void spark_tilemap_set_tile(
        SparkGameComponent* tilemap,
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint16_t tileId) {
    auto* tm = AsComponent<Spark::TilemapComponent>(tilemap, Spark::ComponentKind::Tilemap);
    if (tm != nullptr) {
        tm->SetTile(x, y, tileId);
    }
}

void spark_point_light_set_color(SparkGameComponent* light, const SparkVector3* color) {
    auto* pl = AsComponent<Spark::PointLightComponent>(light, Spark::ComponentKind::PointLight);
    if (pl == nullptr || color == nullptr) {
        return;
    }
    pl->SetColor(ToVector3(*color));
}

void spark_point_light_set_intensity(SparkGameComponent* light, const float intensity) {
    auto* pl = AsComponent<Spark::PointLightComponent>(light, Spark::ComponentKind::PointLight);
    if (pl != nullptr) {
        pl->SetIntensity(intensity);
    }
}

void spark_point_light_set_range(SparkGameComponent* light, const float range) {
    auto* pl = AsComponent<Spark::PointLightComponent>(light, Spark::ComponentKind::PointLight);
    if (pl != nullptr) {
        pl->SetRange(range);
    }
}

void spark_point_light_set_enabled(SparkGameComponent* light, const int enabled) {
    auto* pl = AsComponent<Spark::PointLightComponent>(light, Spark::ComponentKind::PointLight);
    if (pl != nullptr) {
        pl->SetEnabled(enabled != 0);
    }
}

void spark_spot_light_set_color(SparkGameComponent* light, const SparkVector3* color) {
    auto* sl = AsComponent<Spark::SpotLightComponent>(light, Spark::ComponentKind::SpotLight);
    if (sl == nullptr || color == nullptr) {
        return;
    }
    sl->SetColor(ToVector3(*color));
}

void spark_spot_light_set_intensity(SparkGameComponent* light, const float intensity) {
    auto* sl = AsComponent<Spark::SpotLightComponent>(light, Spark::ComponentKind::SpotLight);
    if (sl != nullptr) {
        sl->SetIntensity(intensity);
    }
}

void spark_spot_light_set_range(SparkGameComponent* light, const float range) {
    auto* sl = AsComponent<Spark::SpotLightComponent>(light, Spark::ComponentKind::SpotLight);
    if (sl != nullptr) {
        sl->SetRange(range);
    }
}

void spark_spot_light_set_cones(
        SparkGameComponent* light,
        const float innerConeDegrees,
        const float outerConeDegrees) {
    auto* sl = AsComponent<Spark::SpotLightComponent>(light, Spark::ComponentKind::SpotLight);
    if (sl != nullptr) {
        sl->SetInnerConeDegrees(innerConeDegrees);
        sl->SetOuterConeDegrees(outerConeDegrees);
    }
}

void spark_spot_light_set_enabled(SparkGameComponent* light, const int enabled) {
    auto* sl = AsComponent<Spark::SpotLightComponent>(light, Spark::ComponentKind::SpotLight);
    if (sl != nullptr) {
        sl->SetEnabled(enabled != 0);
    }
}

void spark_sky_set_mode(SparkGameComponent* sky, const SparkSceneSkyMode mode) {
    auto* sc = AsComponent<Spark::SkyComponent>(sky, Spark::ComponentKind::Sky);
    if (sc != nullptr) {
        sc->SetSkyMode(ToCppSkyMode(mode));
    }
}

void spark_sky_set_tint(SparkGameComponent* sky, const SparkVector3* tint) {
    auto* sc = AsComponent<Spark::SkyComponent>(sky, Spark::ComponentKind::Sky);
    if (sc == nullptr || tint == nullptr) {
        return;
    }
    sc->SetTint(ToVector3(*tint));
}

void spark_sky_set_enabled(SparkGameComponent* sky, const int enabled) {
    auto* sc = AsComponent<Spark::SkyComponent>(sky, Spark::ComponentKind::Sky);
    if (sc != nullptr) {
        sc->SetSkyEnabled(enabled != 0);
    }
}

void spark_text_overlay_set_text(SparkGameComponent* text, const char* utf8) {
    auto* to = AsComponent<Spark::TextOverlayComponent>(text, Spark::ComponentKind::TextOverlay);
    if (to == nullptr) {
        return;
    }
    to->SetText(utf8 != nullptr ? Spark::Utf8String(utf8) : Spark::Utf8String());
}

void spark_text_overlay_set_screen_position(SparkGameComponent* text, const float x, const float y) {
    auto* to = AsComponent<Spark::TextOverlayComponent>(text, Spark::ComponentKind::TextOverlay);
    if (to != nullptr) {
        to->SetScreenPosition(x, y);
    }
}

void spark_text_overlay_set_font_size_pixels(SparkGameComponent* text, const float px) {
    auto* to = AsComponent<Spark::TextOverlayComponent>(text, Spark::ComponentKind::TextOverlay);
    if (to != nullptr) {
        to->SetFontSizePixels(px);
    }
}

void spark_text_overlay_set_color(SparkGameComponent* text, const SparkVector3* rgb) {
    auto* to = AsComponent<Spark::TextOverlayComponent>(text, Spark::ComponentKind::TextOverlay);
    if (to == nullptr || rgb == nullptr) {
        return;
    }
    to->SetColor(ToVector3(*rgb));
}

void spark_text_overlay_set_alpha(SparkGameComponent* text, const float alpha) {
    auto* to = AsComponent<Spark::TextOverlayComponent>(text, Spark::ComponentKind::TextOverlay);
    if (to != nullptr) {
        to->SetAlpha(alpha);
    }
}

void spark_text_overlay_set_visible(SparkGameComponent* text, const int visible) {
    auto* to = AsComponent<Spark::TextOverlayComponent>(text, Spark::ComponentKind::TextOverlay);
    if (to != nullptr) {
        to->SetVisible(visible != 0);
    }
}

void spark_gui_canvas_set_enabled(SparkGameComponent* canvas, const int enabled) {
    auto* gui = AsComponent<Spark::GuiCanvasComponent>(canvas, Spark::ComponentKind::GuiCanvas);
    if (gui != nullptr) {
        gui->SetCanvasEnabled(enabled != 0);
    }
}

void spark_gui_canvas_set_sort_order(SparkGameComponent* canvas, const int sortOrder) {
    auto* gui = AsComponent<Spark::GuiCanvasComponent>(canvas, Spark::ComponentKind::GuiCanvas);
    if (gui != nullptr) {
        gui->SetSortOrder(sortOrder);
    }
}

void spark_gui_canvas_clear_root(SparkGameComponent* canvas) {
    auto* gui = AsComponent<Spark::GuiCanvasComponent>(canvas, Spark::ComponentKind::GuiCanvas);
    if (gui != nullptr) {
        gui->SetRoot(Spark::UniquePtr<Spark::Gui::Panel>());
    }
}

void spark_rigidbody_2d_get_velocity(const SparkGameComponent* body, SparkVector2* out) {
    const auto* rb = AsComponent<const Spark::Rigidbody2DComponent>(body, Spark::ComponentKind::Rigidbody2D);
    if (rb == nullptr || out == nullptr) {
        return;
    }
    *out = FromVector2(rb->GetVelocity());
}

void spark_rigidbody_2d_set_velocity(SparkGameComponent* body, const SparkVector2* v) {
    auto* rb = AsComponent<Spark::Rigidbody2DComponent>(body, Spark::ComponentKind::Rigidbody2D);
    if (rb == nullptr || v == nullptr) {
        return;
    }
    rb->SetVelocity(ToVector2(*v));
}

int spark_rigidbody_2d_is_grounded(const SparkGameComponent* body) {
    const auto* rb = AsComponent<const Spark::Rigidbody2DComponent>(body, Spark::ComponentKind::Rigidbody2D);
    return (rb != nullptr && rb->IsGrounded()) ? 1 : 0;
}

void spark_rigidbody_3d_get_velocity(const SparkGameComponent* body, SparkVector3* out) {
    const auto* rb = AsComponent<const Spark::Rigidbody3DComponent>(body, Spark::ComponentKind::Rigidbody3D);
    if (rb == nullptr || out == nullptr) {
        return;
    }
    *out = FromVector3(rb->GetVelocity());
}

void spark_rigidbody_3d_set_velocity(SparkGameComponent* body, const SparkVector3* v) {
    auto* rb = AsComponent<Spark::Rigidbody3DComponent>(body, Spark::ComponentKind::Rigidbody3D);
    if (rb == nullptr || v == nullptr) {
        return;
    }
    rb->SetVelocity(ToVector3(*v));
}

void spark_rigidbody_3d_get_angular_velocity(const SparkGameComponent* body, SparkVector3* out) {
    const auto* rb = AsComponent<const Spark::Rigidbody3DComponent>(body, Spark::ComponentKind::Rigidbody3D);
    if (rb == nullptr || out == nullptr) {
        return;
    }
    *out = FromVector3(rb->GetAngularVelocity());
}

void spark_rigidbody_3d_set_angular_velocity(SparkGameComponent* body, const SparkVector3* v) {
    auto* rb = AsComponent<Spark::Rigidbody3DComponent>(body, Spark::ComponentKind::Rigidbody3D);
    if (rb == nullptr || v == nullptr) {
        return;
    }
    rb->SetAngularVelocity(ToVector3(*v));
}

void spark_rigidbody_3d_set_linear_damping(SparkGameComponent* body, const float v) {
    auto* rb = AsComponent<Spark::Rigidbody3DComponent>(body, Spark::ComponentKind::Rigidbody3D);
    if (rb != nullptr) {
        rb->SetLinearDamping(v);
    }
}

void spark_rigidbody_3d_set_angular_damping(SparkGameComponent* body, const float v) {
    auto* rb = AsComponent<Spark::Rigidbody3DComponent>(body, Spark::ComponentKind::Rigidbody3D);
    if (rb != nullptr) {
        rb->SetAngularDamping(v);
    }
}

void spark_rigidbody_3d_set_restitution(SparkGameComponent* body, const float v) {
    auto* rb = AsComponent<Spark::Rigidbody3DComponent>(body, Spark::ComponentKind::Rigidbody3D);
    if (rb != nullptr) {
        rb->SetRestitution(v);
    }
}

void spark_rigidbody_3d_set_inverse_mass(SparkGameComponent* body, const float inverseMass) {
    auto* rb = AsComponent<Spark::Rigidbody3DComponent>(body, Spark::ComponentKind::Rigidbody3D);
    if (rb != nullptr) {
        rb->SetInverseMass(inverseMass);
    }
}

void spark_box_collider_2d_set_is_trigger(SparkGameComponent* collider, const int isTrigger) {
    auto* box = AsComponent<Spark::BoxCollider2DComponent>(collider, Spark::ComponentKind::BoxCollider2D);
    if (box != nullptr) {
        box->SetIsTrigger(isTrigger != 0);
    }
}

void spark_box_collider_2d_set_half_extents(SparkGameComponent* collider, const SparkVector2* halfExtents) {
    auto* box = AsComponent<Spark::BoxCollider2DComponent>(collider, Spark::ComponentKind::BoxCollider2D);
    if (box == nullptr || halfExtents == nullptr) {
        return;
    }
    box->SetHalfExtents(ToVector2(*halfExtents));
}

void spark_box_collider_2d_set_category_bits(SparkGameComponent* collider, const uint16_t bits) {
    auto* box = AsComponent<Spark::BoxCollider2DComponent>(collider, Spark::ComponentKind::BoxCollider2D);
    if (box != nullptr) {
        box->SetCategoryBits(bits);
    }
}

void spark_box_collider_2d_set_mask_bits(SparkGameComponent* collider, const uint16_t bits) {
    auto* box = AsComponent<Spark::BoxCollider2DComponent>(collider, Spark::ComponentKind::BoxCollider2D);
    if (box != nullptr) {
        box->SetMaskBits(bits);
    }
}

void spark_circle_collider_2d_set_is_trigger(SparkGameComponent* collider, const int isTrigger) {
    auto* circle = AsComponent<Spark::CircleCollider2DComponent>(collider, Spark::ComponentKind::CircleCollider2D);
    if (circle != nullptr) {
        circle->SetIsTrigger(isTrigger != 0);
    }
}

void spark_circle_collider_2d_set_radius(SparkGameComponent* collider, const float radius) {
    auto* circle = AsComponent<Spark::CircleCollider2DComponent>(collider, Spark::ComponentKind::CircleCollider2D);
    if (circle != nullptr) {
        circle->SetRadius(radius);
    }
}

void spark_circle_collider_2d_set_category_bits(SparkGameComponent* collider, const uint16_t bits) {
    auto* circle = AsComponent<Spark::CircleCollider2DComponent>(collider, Spark::ComponentKind::CircleCollider2D);
    if (circle != nullptr) {
        circle->SetCategoryBits(bits);
    }
}

void spark_circle_collider_2d_set_mask_bits(SparkGameComponent* collider, const uint16_t bits) {
    auto* circle = AsComponent<Spark::CircleCollider2DComponent>(collider, Spark::ComponentKind::CircleCollider2D);
    if (circle != nullptr) {
        circle->SetMaskBits(bits);
    }
}

std::uint32_t spark_animator_get_clip_index(const SparkGameComponent* animator) {
    const auto* anim = AsComponent<const Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    return anim != nullptr ? anim->GetClipIndex() : 0;
}

float spark_animator_get_time_seconds(const SparkGameComponent* animator) {
    const auto* anim = AsComponent<const Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    return anim != nullptr ? anim->GetTimeSeconds() : 0.0F;
}

float spark_animator_get_speed(const SparkGameComponent* animator) {
    const auto* anim = AsComponent<const Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    return anim != nullptr ? anim->GetSpeed() : 0.0F;
}

void spark_animator_set_clip_index(SparkGameComponent* animator, const std::uint32_t clipIndex) {
    auto* anim = AsComponent<Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    if (anim != nullptr) {
        anim->SetClipIndex(clipIndex);
    }
}

void spark_animator_set_speed(SparkGameComponent* animator, const float speed) {
    auto* anim = AsComponent<Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    if (anim != nullptr) {
        anim->SetSpeed(speed);
    }
}

void spark_animator_set_time_seconds(SparkGameComponent* animator, const float timeSeconds) {
    auto* anim = AsComponent<Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    if (anim != nullptr) {
        anim->SetTimeSeconds(timeSeconds);
    }
}

std::uint32_t spark_animator_get_clip_count(const SparkGameComponent* animator) {
    const auto* anim = AsComponent<const Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    return anim != nullptr ? anim->GetClipCount() : 0;
}

std::uint32_t spark_animator_get_loop_mode(const SparkGameComponent* animator) {
    const auto* anim = AsComponent<const Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    return anim != nullptr ? static_cast<std::uint32_t>(anim->GetLoopMode()) : 0;
}

void spark_animator_set_loop_mode(SparkGameComponent* animator, const std::uint32_t loopMode) {
    auto* anim = AsComponent<Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    if (anim != nullptr) {
        anim->SetLoopMode(static_cast<Spark::AnimLoopMode>(loopMode));
    }
}

int spark_animator_is_clip_finished(const SparkGameComponent* animator) {
    const auto* anim = AsComponent<const Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    return (anim != nullptr && anim->IsClipFinished()) ? 1 : 0;
}

void spark_animator_set_clip_index_with_crossfade(
        SparkGameComponent* animator,
        const std::uint32_t clipIndex,
        const float crossfadeDurationSec) {
    auto* anim = AsComponent<Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    if (anim != nullptr) {
        anim->SetClipIndexWithCrossfade(clipIndex, crossfadeDurationSec);
    }
}

std::int32_t spark_animator_find_clip_index_by_name(const SparkGameComponent* animator, const char* name) {
    const auto* anim = AsComponent<const Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    return anim != nullptr ? anim->FindClipIndexByName(name) : -1;
}

int spark_animator_get_clip_name(
        const SparkGameComponent* animator,
        const std::uint32_t clipIndex,
        char* outUtf8,
        const std::uint32_t outCapacity) {
    if (outUtf8 == nullptr || outCapacity == 0) {
        return 0;
    }
    outUtf8[0] = '\0';
    const auto* anim = AsComponent<const Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    if (anim == nullptr) {
        return 0;
    }
    const Spark::Utf8String& name = anim->GetClipName(clipIndex);
    const char* src = name.CStr();
    if (src == nullptr) {
        return 0;
    }
    std::uint32_t i = 0;
    for (; src[i] != '\0' && i + 1 < outCapacity; ++i) {
        outUtf8[i] = src[i];
    }
    outUtf8[i] = '\0';
    return static_cast<int>(i);
}

void spark_sprite_animator_set_uniform_grid(
        SparkGameComponent* animator,
        const uint32_t columns,
        const uint32_t rows) {
    auto* sa = AsComponent<Spark::SpriteAnimatorComponent>(animator, Spark::ComponentKind::SpriteAnimator);
    if (sa != nullptr) {
        sa->SetUniformGrid(columns, rows);
    }
}

void spark_sprite_animator_clear_clips(SparkGameComponent* animator) {
    auto* sa = AsComponent<Spark::SpriteAnimatorComponent>(animator, Spark::ComponentKind::SpriteAnimator);
    if (sa != nullptr) {
        sa->ClearClips();
    }
}

void spark_sprite_animator_add_clip(SparkGameComponent* animator, const SparkSpriteAnimationClip* clip) {
    auto* sa = AsComponent<Spark::SpriteAnimatorComponent>(animator, Spark::ComponentKind::SpriteAnimator);
    if (sa == nullptr || clip == nullptr) {
        return;
    }
    Spark::SpriteAnimationClip cpp{
            .firstFrame = clip->firstFrame,
            .frameCount = clip->frameCount,
            .framesPerSecond = clip->framesPerSecond,
            .loop = clip->loop != 0,
    };
    sa->AddClip(cpp);
}

void spark_sprite_animator_set_clip_index(SparkGameComponent* animator, const uint32_t clipIndex) {
    auto* sa = AsComponent<Spark::SpriteAnimatorComponent>(animator, Spark::ComponentKind::SpriteAnimator);
    if (sa != nullptr) {
        sa->SetClipIndex(clipIndex);
    }
}

uint32_t spark_sprite_animator_get_clip_index(const SparkGameComponent* animator) {
    const auto* sa =
            AsComponent<const Spark::SpriteAnimatorComponent>(animator, Spark::ComponentKind::SpriteAnimator);
    return sa != nullptr ? sa->GetClipIndex() : 0U;
}

uint32_t spark_sprite_animator_get_clip_count(const SparkGameComponent* animator) {
    const auto* sa =
            AsComponent<const Spark::SpriteAnimatorComponent>(animator, Spark::ComponentKind::SpriteAnimator);
    return sa != nullptr ? sa->GetClipCount() : 0U;
}

int spark_sprite_animator_is_current_clip_finished(const SparkGameComponent* animator) {
    const auto* sa =
            AsComponent<const Spark::SpriteAnimatorComponent>(animator, Spark::ComponentKind::SpriteAnimator);
    return (sa != nullptr && sa->IsCurrentClipFinished()) ? 1 : 0;
}

void spark_sprite_animator_compute_uniform_grid_uv(
        const uint32_t columns,
        const uint32_t rows,
        const uint32_t linearFrame,
        SparkVector4* outUvRect) {
    if (outUvRect == nullptr) {
        return;
    }
    *outUvRect = FromVector4(Spark::SpriteAnimatorComponent::ComputeUniformGridUv(columns, rows, linearFrame));
}

void spark_sprite_2d_fsm_set_locomotion_clips(
        SparkGameComponent* fsm,
        const uint32_t idleClipIndex,
        const uint32_t moveClipIndex) {
    auto* driver = AsComponent<Spark::Sprite2DCharacterAnimFsmComponent>(
            fsm, Spark::ComponentKind::Sprite2DCharacterAnimFsm);
    if (driver != nullptr) {
        driver->SetLocomotionClips(idleClipIndex, moveClipIndex);
    }
}

void spark_sprite_2d_fsm_set_combat_clips(
        SparkGameComponent* fsm,
        const uint32_t attackClipIndex,
        const uint32_t hurtClipIndex) {
    auto* driver = AsComponent<Spark::Sprite2DCharacterAnimFsmComponent>(
            fsm, Spark::ComponentKind::Sprite2DCharacterAnimFsm);
    if (driver != nullptr) {
        driver->SetCombatClips(attackClipIndex, hurtClipIndex);
    }
}

void spark_sprite_2d_fsm_set_move_speed_threshold(SparkGameComponent* fsm, const float worldUnits) {
    auto* driver = AsComponent<Spark::Sprite2DCharacterAnimFsmComponent>(
            fsm, Spark::ComponentKind::Sprite2DCharacterAnimFsm);
    if (driver != nullptr) {
        driver->SetMoveSpeedThreshold(worldUnits);
    }
}

void spark_sprite_2d_fsm_set_locomotion_source(
        SparkGameComponent* fsm,
        const SparkSprite2DAnimLocomotionSource source) {
    auto* driver = AsComponent<Spark::Sprite2DCharacterAnimFsmComponent>(
            fsm, Spark::ComponentKind::Sprite2DCharacterAnimFsm);
    if (driver != nullptr) {
        driver->SetLocomotionSource(static_cast<Spark::Sprite2DAnimLocomotionSource>(source));
    }
}

void spark_sprite_2d_fsm_request_hurt(SparkGameComponent* fsm) {
    auto* driver = AsComponent<Spark::Sprite2DCharacterAnimFsmComponent>(
            fsm, Spark::ComponentKind::Sprite2DCharacterAnimFsm);
    if (driver != nullptr) {
        driver->RequestHurt();
    }
}

void spark_sprite_2d_fsm_request_attack(SparkGameComponent* fsm) {
    auto* driver = AsComponent<Spark::Sprite2DCharacterAnimFsmComponent>(
            fsm, Spark::ComponentKind::Sprite2DCharacterAnimFsm);
    if (driver != nullptr) {
        driver->RequestAttack();
    }
}

void spark_char_3d_fsm_set_locomotion_clips(
        SparkGameComponent* fsm,
        const std::uint32_t idleClipIndex,
        const std::uint32_t walkClipIndex,
        const std::uint32_t runClipIndex) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->SetLocomotionClips(idleClipIndex, walkClipIndex, runClipIndex);
    }
}

void spark_char_3d_fsm_configure_locomotion_from_skeleton(
        SparkGameComponent* fsm,
        const SparkGameComponent* animator,
        const std::uint32_t walkClipFallback) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    const auto* anim = AsComponent<const Spark::AnimatorComponent>(animator, Spark::ComponentKind::Animator);
    if (driver == nullptr || anim == nullptr || !anim->GetSkeleton()) {
        return;
    }
    driver->ConfigureLocomotionFromSkeleton(*anim->GetSkeleton(), walkClipFallback);
}

void spark_char_3d_fsm_set_attack_clip(SparkGameComponent* fsm, const std::uint32_t attackClipIndex) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->SetAttackClip(attackClipIndex);
    }
}

void spark_char_3d_fsm_set_walk_speed_threshold(SparkGameComponent* fsm, const float metersPerSecond) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->SetWalkSpeedThreshold(metersPerSecond);
    }
}

void spark_char_3d_fsm_set_run_speed_threshold(SparkGameComponent* fsm, const float metersPerSecond) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->SetRunSpeedThreshold(metersPerSecond);
    }
}

void spark_char_3d_fsm_set_crossfade_duration(SparkGameComponent* fsm, const float seconds) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->SetCrossfadeDuration(seconds);
    }
}

void spark_char_3d_fsm_set_locomotion_driving_enabled(SparkGameComponent* fsm, const int enabled) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->SetLocomotionDrivingEnabled(enabled != 0);
    }
}

void spark_char_3d_fsm_set_locomotion_input(
        SparkGameComponent* fsm,
        const int moving,
        const int sprint) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->SetLocomotionInput(moving != 0, sprint != 0);
    }
}

void spark_char_3d_fsm_set_manual_clip(
        SparkGameComponent* fsm,
        const std::uint32_t clipIndex,
        const std::uint32_t loopMode) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->SetManualClip(clipIndex, static_cast<Spark::AnimLoopMode>(loopMode));
    }
}

void spark_char_3d_fsm_clear_manual_clip(SparkGameComponent* fsm) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->ClearManualClip();
    }
}

void spark_char_3d_fsm_request_attack(SparkGameComponent* fsm) {
    auto* driver = AsComponent<Spark::Character3DAnimFsmComponent>(
            fsm, Spark::ComponentKind::Character3DAnimFsm);
    if (driver != nullptr) {
        driver->RequestAttack();
    }
}

void spark_particle_emitter_set_enabled(SparkGameComponent* emitter, const int enabled) {
    auto* pe = AsComponent<Spark::ParticleEmitterComponent>(emitter, Spark::ComponentKind::ParticleEmitter);
    if (pe != nullptr) {
        pe->SetEmitterEnabled(enabled != 0);
    }
}

void spark_particle_emitter_set_rate(SparkGameComponent* emitter, const float particlesPerSecond) {
    auto* pe = AsComponent<Spark::ParticleEmitterComponent>(emitter, Spark::ComponentKind::ParticleEmitter);
    if (pe != nullptr) {
        pe->SetEmissionRate(particlesPerSecond);
    }
}

void spark_particle_emitter_set_max_particles(SparkGameComponent* emitter, const int maxParticles) {
    auto* pe = AsComponent<Spark::ParticleEmitterComponent>(emitter, Spark::ComponentKind::ParticleEmitter);
    if (pe != nullptr && maxParticles >= 0) {
        pe->SetMaxParticles(static_cast<std::uint32_t>(maxParticles));
    }
}

}  // extern "C"
