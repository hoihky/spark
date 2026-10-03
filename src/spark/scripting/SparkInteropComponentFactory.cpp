#include "spark/scripting/SparkInterop.h"
#include "spark/scripting/SparkInteropInternal.hpp"

#include "spark/ecs/GameObject.hpp"

#include "SparkInteropComponentFactory.generated.cpp"

namespace {

Spark::GameObject* ToObject(SparkGameObject* object) {
    return reinterpret_cast<Spark::GameObject*>(object);
}

}  // namespace

extern "C" {

SparkComponentKind spark_component_get_kind(const SparkGameComponent* component) {
    if (component == nullptr) {
        return SparkComponentKind_Unknown;
    }
    const auto* base = reinterpret_cast<const Spark::GameComponent*>(component);
    return static_cast<SparkComponentKind>(static_cast<std::uint32_t>(base->Kind()));
}

SparkGameComponent* spark_object_get_or_add_default_component(SparkGameObject* object, SparkComponentKind kind) {
    if (object == nullptr || kind == SparkComponentKind_Unknown) {
        return nullptr;
    }
    return SparkInteropFactory::DispatchGetOrAddDefault(ToObject(object), kind);
}

}  // extern "C"
