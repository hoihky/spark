#include "spark/editor/commands/CreateGameObjectCommand.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"

namespace Spark::Editor {

CreateGameObjectCommand::CreateGameObjectCommand(
        GameWorld& inWorld,
        SceneEditorContentModel& inContent,
        Utf8String name,
        GameObject* parent)
    : world(&inWorld),
      content(&inContent),
      objectName(MoveTemp(name)),
      parentObjectId(parent != nullptr ? parent->GetId() : 0) {}

void CreateGameObjectCommand::CreateObject() {
    if (world == nullptr || content == nullptr) {
        return;
    }
    created = world->CreateGameObject();
    created->GetName() = objectName;
    created->AddComponent<TransformComponent>();
    GameObject* parent = parentObjectId != 0 ? world->FindGameObjectById(parentObjectId) : nullptr;
    (void)world->SetParent(created, parent);
    content->TrackRoot(created);
}

void CreateGameObjectCommand::DestroyCreated() {
    if (world == nullptr || content == nullptr || created == nullptr) {
        return;
    }
    content->UntrackSubtree(*created);
    world->DestroyGameObject(created);
    created = nullptr;
}

void CreateGameObjectCommand::Undo() {
    DestroyCreated();
}

void CreateGameObjectCommand::Redo() {
    CreateObject();
}

Utf8String CreateGameObjectCommand::GetDescription() const {
    return Utf8String("Create GameObject");
}

}  // namespace Spark::Editor
