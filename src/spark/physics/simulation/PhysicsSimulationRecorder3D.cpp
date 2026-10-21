#include "spark/physics/simulation/PhysicsSimulationRecorder3D.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/colliders/DynamicBody3D.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void PhysicsReplayRingBuffer3D::SetCapacity(const std::size_t frameCapacity) noexcept {
    capacity = frameCapacity < 1 ? 1 : frameCapacity;
    while (frames.GetSize() > capacity) {
        frames.PopBack();
    }
}

void PhysicsReplayRingBuffer3D::Clear() noexcept {
    frames.Clear();
}

void PhysicsReplayRingBuffer3D::CaptureFrame(GameWorld& world) {
    Array<DynamicBody3D> bodies;
    CollectDynamicBodies3D(world, bodies);

    Array<PhysicsRigidbodySnapshot3D> frame;
    for (std::size_t i = 0; i < bodies.GetSize(); ++i) {
        const DynamicBody3D& body = bodies[i];
        if (body.obj == nullptr || body.rb == nullptr || body.tr == nullptr) {
            continue;
        }
        PhysicsRigidbodySnapshot3D row{};
        row.objectId = body.obj->GetId();
        row.translation = body.tr->GetLocalTransform().translation;
        row.velocity = body.rb->GetVelocity();
        row.angularVelocity = body.rb->GetAngularVelocity();
        frame.PushBack(row);
    }

    if (frames.GetSize() >= capacity) {
        frames.RemoveAt(0);
    }
    frames.PushBack(MoveTemp(frame));
}

bool PhysicsReplayRingBuffer3D::RestoreFrame(
        const std::size_t frameIndexFromLatest,
        GameWorld& world) const noexcept {
    if (frames.GetSize() == 0 || frameIndexFromLatest >= frames.GetSize()) {
        return false;
    }
    const std::size_t index = frames.GetSize() - 1 - frameIndexFromLatest;
    const Array<PhysicsRigidbodySnapshot3D>& frame = frames[index];
    for (std::size_t i = 0; i < frame.GetSize(); ++i) {
        const PhysicsRigidbodySnapshot3D& row = frame[i];
        GameObject* object = world.FindGameObjectById(row.objectId);
        if (object == nullptr) {
            continue;
        }
        if (auto* tr = object->GetComponent<TransformComponent>()) {
            tr->SetTranslation(row.translation);
        }
        if (auto* rb = object->GetComponent<Rigidbody3DComponent>()) {
            rb->SetVelocity(row.velocity);
            rb->SetAngularVelocity(row.angularVelocity);
        }
    }
    return true;
}

void PhysicsSimulationDomain3D::OnLiveStepFinished(GameWorld& world) {
    if (!recordingEnabled || mode != AdvanceMode::Live) {
        return;
    }
    replayBuffer.CaptureFrame(world);
}

bool PhysicsSimulationDomain3D::PlaybackFrameFromLatest(
        const std::size_t framesBack,
        GameWorld& world) const noexcept {
    return replayBuffer.RestoreFrame(framesBack, world);
}

}  // namespace Spark
