#include "spark/scripting/SparkInterop.h"
#include "spark/scripting/SparkInteropInternal.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/foliage/FoliageInstancedMeshComponent.hpp"
#include "spark/ecs/components/foliage/GrassFieldComponent.hpp"
#include "spark/ecs/components/foliage/WindEnvironmentComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace {

Spark::GameObject* ToObject(SparkGameObject* object) {
    return reinterpret_cast<Spark::GameObject*>(object);
}

Spark::GameWorld* ToWorld(SparkGameWorld* world) {
    return reinterpret_cast<Spark::GameWorld*>(world);
}

Spark::FoliageInstancedMeshComponent* AsFoliageInstanced(SparkGameComponent* component) {
    return Spark::Scripting::AsComponent<Spark::FoliageInstancedMeshComponent>(
            component, Spark::ComponentKind::FoliageInstancedMesh);
}

const Spark::FoliageInstancedMeshComponent* AsFoliageInstanced(const SparkGameComponent* component) {
    return Spark::Scripting::AsComponent<const Spark::FoliageInstancedMeshComponent>(
            component, Spark::ComponentKind::FoliageInstancedMesh);
}

Spark::GrassFieldComponent* AsGrassField(SparkGameComponent* component) {
    return Spark::Scripting::AsComponent<Spark::GrassFieldComponent>(
            component, Spark::ComponentKind::GrassField);
}

const Spark::GrassFieldComponent* AsGrassField(const SparkGameComponent* component) {
    return Spark::Scripting::AsComponent<const Spark::GrassFieldComponent>(
            component, Spark::ComponentKind::GrassField);
}

int SetMeshKey(
        Spark::FoliageInstancedMeshComponent& foliage,
        Spark::GameWorld& world,
        const char* meshKeyOrPath) {
    if (meshKeyOrPath == nullptr) {
        return 0;
    }
    Spark::SharedPtr<Spark::Mesh> mesh = world.TryGetMeshByKeyOrPath(meshKeyOrPath);
    if (!mesh) {
        return 0;
    }
    foliage.SetBladeMesh(mesh);
    return 1;
}

int SetTextureKey(
        Spark::FoliageInstancedMeshComponent& foliage,
        Spark::GameWorld& world,
        const char* textureKeyOrPath) {
    if (textureKeyOrPath == nullptr) {
        return 0;
    }
    Spark::SharedPtr<Spark::Texture2D> tex = world.TryGetTextureByKeyOrPath(textureKeyOrPath);
    if (!tex) {
        return 0;
    }
    foliage.SetAlbedoTexture(tex);
    return 1;
}

}  // namespace

extern "C" {

SparkGameComponent* spark_object_add_wind_environment(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::WindEnvironmentComponent>());
}

SparkGameComponent* spark_object_add_foliage_instanced_mesh(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::FoliageInstancedMeshComponent>());
}

void spark_foliage_instanced_set_enabled(SparkGameComponent* foliage, const int enabled) {
    auto* c = AsFoliageInstanced(foliage);
    if (c != nullptr) {
        c->SetEnabled(enabled != 0);
    }
}

int spark_foliage_instanced_get_enabled(const SparkGameComponent* foliage) {
    const auto* c = AsFoliageInstanced(foliage);
    return c != nullptr && c->IsEnabled() ? 1 : 0;
}

int spark_foliage_instanced_set_blade_mesh_key(
        SparkGameComponent* foliage,
        SparkGameWorld* world,
        const char* meshKeyOrPath) {
    if (foliage == nullptr || world == nullptr) {
        return 0;
    }
    auto* c = AsFoliageInstanced(foliage);
    return c != nullptr ? SetMeshKey(*c, *ToWorld(world), meshKeyOrPath) : 0;
}

int spark_foliage_instanced_set_albedo_texture_key(
        SparkGameComponent* foliage,
        SparkGameWorld* world,
        const char* textureKeyOrPath) {
    if (foliage == nullptr || world == nullptr) {
        return 0;
    }
    auto* c = AsFoliageInstanced(foliage);
    return c != nullptr ? SetTextureKey(*c, *ToWorld(world), textureKeyOrPath) : 0;
}

void spark_foliage_instanced_get_albedo_tint(const SparkGameComponent* foliage, SparkVector3* outRgb) {
    if (outRgb == nullptr) {
        return;
    }
    const auto* c = AsFoliageInstanced(foliage);
    *outRgb = c != nullptr ? Spark::Scripting::FromVector3(c->GetAlbedoTint())
                           : SparkVector3{0.5F, 0.9F, 0.4F};
}

void spark_foliage_instanced_set_albedo_tint(SparkGameComponent* foliage, const SparkVector3* rgb) {
    if (rgb == nullptr) {
        return;
    }
    auto* c = AsFoliageInstanced(foliage);
    if (c != nullptr) {
        c->SetAlbedoTint(Spark::Scripting::ToVector3(*rgb));
    }
}

float spark_foliage_instanced_get_alpha_cutoff(const SparkGameComponent* foliage) {
    const auto* c = AsFoliageInstanced(foliage);
    return c != nullptr ? c->GetAlphaCutoff() : 0.35F;
}

void spark_foliage_instanced_set_alpha_cutoff(SparkGameComponent* foliage, const float value) {
    auto* c = AsFoliageInstanced(foliage);
    if (c != nullptr) {
        c->SetAlphaCutoff(value);
    }
}

float spark_foliage_instanced_get_wind_bend_scale(const SparkGameComponent* foliage) {
    const auto* c = AsFoliageInstanced(foliage);
    return c != nullptr ? c->GetWindBendScale() : 0.28F;
}

void spark_foliage_instanced_set_wind_bend_scale(SparkGameComponent* foliage, const float scale) {
    auto* c = AsFoliageInstanced(foliage);
    if (c != nullptr) {
        c->SetWindBendScale(scale);
    }
}

void spark_foliage_instanced_configure_grid(
        SparkGameComponent* foliage,
        const int columns,
        const int rows,
        const float spacingMeters) {
    auto* c = AsFoliageInstanced(foliage);
    if (c != nullptr) {
        c->ConfigureGrid(columns, rows, spacingMeters);
    }
}

void spark_foliage_instanced_configure_grid_within_square(
        SparkGameComponent* foliage,
        const float halfExtentMeters,
        const float spacingMeters,
        const float edgeMarginMeters) {
    auto* c = AsFoliageInstanced(foliage);
    if (c != nullptr) {
        c->ConfigureGridWithinSquare(halfExtentMeters, spacingMeters, edgeMarginMeters);
    }
}

SparkGameComponent* spark_object_add_grass_field(SparkGameObject* object) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(
            ToObject(object)->AddComponent<Spark::GrassFieldComponent>());
}

int spark_grass_field_set_blade_mesh_key(
        SparkGameComponent* grass,
        SparkGameWorld* world,
        const char* meshKeyOrPath) {
    if (grass == nullptr || world == nullptr) {
        return 0;
    }
    auto* c = AsGrassField(grass);
    if (c == nullptr) {
        return 0;
    }
    if (meshKeyOrPath == nullptr) {
        return 0;
    }
    Spark::SharedPtr<Spark::Mesh> mesh = ToWorld(world)->TryGetMeshByKeyOrPath(meshKeyOrPath);
    if (!mesh) {
        return 0;
    }
    c->SetBladeMesh(mesh);
    return 1;
}

int spark_grass_field_set_albedo_texture_key(
        SparkGameComponent* grass,
        SparkGameWorld* world,
        const char* textureKeyOrPath) {
    if (grass == nullptr || world == nullptr) {
        return 0;
    }
    auto* c = AsGrassField(grass);
    if (c == nullptr) {
        return 0;
    }
    if (textureKeyOrPath == nullptr) {
        return 0;
    }
    Spark::SharedPtr<Spark::Texture2D> tex = ToWorld(world)->TryGetTextureByKeyOrPath(textureKeyOrPath);
    if (!tex) {
        return 0;
    }
    c->SetAlbedoTexture(tex);
    return 1;
}

}  // extern "C"
