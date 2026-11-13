#include "spark/scene/core/world/WorldQuery.hpp"

#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void WorldQuery::Collect(GameObjectQueryResult& out, const GameObjectQueryFilter& filter) const {
    world_.CollectGameObjects(out, filter);
}

GameObjectQueryResult WorldQuery::Query(const GameObjectQueryFilter& filter) const {
    return world_.QueryGameObjects(filter);
}

std::size_t WorldQuery::CollectInto(Array<GameObject*>& out, const GameObjectQueryFilter& filter) const {
    const std::size_t before = out.GetSize();
    world_.ForEachGameObject([&](GameObject* object) { out.PushBack(object); }, filter);
    return out.GetSize() - before;
}

std::size_t WorldQuery::CollectIntoFresh(GameObjectQueryResult& out, const GameObjectQueryFilter& filter) const {
    out.Clear();
    Collect(out, filter);
    return out.GetSize();
}

std::size_t WorldQuery::CollectWithTag(const char* tag, Array<GameObject*>& out, bool includeInactive) const {
    if (tag == nullptr) {
        return 0;
    }
    GameObjectQueryFilter filter;
    filter.includeInactive = includeInactive;
    filter.tagEquals = tag;
    return CollectInto(out, filter);
}

std::size_t WorldQuery::CollectWithTag(const char* tag, GameObjectQueryResult& out, bool includeInactive) const {
    if (tag == nullptr) {
        out.Clear();
        return 0;
    }
    GameObjectQueryFilter filter;
    filter.includeInactive = includeInactive;
    filter.tagEquals = tag;
    out.Clear();
    Collect(out, filter);
    return out.GetSize();
}

GameObject* WorldQuery::FindWithTag(const char* tag, bool includeInactive) const {
    GameObjectQueryFilter filter;
    filter.includeInactive = includeInactive;
    return world_.FindGameObjectWithTag(tag, filter);
}

GameObject* WorldQuery::FindByName(const char* name, bool includeInactive) const {
    GameObjectQueryFilter filter;
    filter.includeInactive = includeInactive;
    return world_.FindGameObjectByName(name, filter);
}

std::size_t WorldQuery::CollectWithComponent(
        ComponentKind kind,
        Array<GameObject*>& out,
        bool includeInactive) const {
    GameObjectQueryFilter filter;
    filter.includeInactive = includeInactive;
    filter.requiredComponent = kind;
    return CollectInto(out, filter);
}

}  // namespace Spark
