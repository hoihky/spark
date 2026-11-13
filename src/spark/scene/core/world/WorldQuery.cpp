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
    return CollectInto(out, QueryFilter::WithTag(tag, includeInactive));
}

std::size_t WorldQuery::CollectWithTag(const char* tag, GameObjectQueryResult& out, bool includeInactive) const {
    if (tag == nullptr) {
        out.Clear();
        return 0;
    }
    out.Clear();
    Collect(out, QueryFilter::WithTag(tag, includeInactive));
    return out.GetSize();
}

GameObject* WorldQuery::FindWithTag(const char* tag, bool includeInactive) const {
    return world_.FindGameObjectWithTag(tag, QueryFilter::WithTag(tag, includeInactive));
}

GameObject* WorldQuery::FindByName(const char* name, bool includeInactive) const {
    GameObjectQueryFilter f = includeInactive ? QueryFilter::IncludeInactive() : QueryFilter::ActiveOnly();
    return world_.FindGameObjectByName(name, f);
}

std::size_t WorldQuery::CollectWithComponent(
        ComponentKind kind,
        Array<GameObject*>& out,
        bool includeInactive) const {
    return CollectInto(out, QueryFilter::WithComponent(kind, includeInactive));
}

std::size_t WorldQuery::CollectRoots(Array<GameObject*>& out, bool includeInactive) const {
    return CollectInto(out, QueryFilter::RootsOnly(includeInactive));
}

std::size_t WorldQuery::CollectScene(
        SceneInstanceId sceneId,
        Array<GameObject*>& out,
        bool includeInactive) const {
    return CollectInto(out, QueryFilter::ForScene(sceneId, includeInactive));
}

}  // namespace Spark
