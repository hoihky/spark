#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/scene/core/EntityId.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace {

class NoOpEngineContext final : public Spark::IEngineContext {
public:
    Spark::Window& GetWindow() override { std::terminate(); }
    Spark::IFramePresenter& GetFramePresenter() override { std::terminate(); }
    Spark::IInput& GetInput() override { std::terminate(); }
    void GetFramebufferSize(int& outWidth, int& outHeight) const override {
        outWidth = 1280;
        outHeight = 720;
    }
    void SetSceneRenderParams(const Spark::SceneRenderParams& /*params*/) override {}
    Spark::SoundEngine* TryGetSoundEngine() noexcept override { return nullptr; }
    Spark::Scene* TryGetScene() noexcept override { return nullptr; }
    Spark::IImGuiLayer* TryGetImGuiLayer() noexcept override { return nullptr; }
};

struct DestroyOnUpdate final : public Spark::GameComponent {
    static constexpr Spark::ComponentKind TypeKind = Spark::ComponentKind::TimeOfDayDriver;

    Spark::ComponentKind Kind() const noexcept override { return TypeKind; }

    Spark::GameObject* target = nullptr;

    void OnUpdate(const Spark::FrameTiming&, Spark::GameObject&, Spark::IEngineContext&) override {
        if (target != nullptr) {
            target->GetWorld().DestroyGameObject(target);
        }
    }
};

NoOpEngineContext gTestEngineContext{};

}  // namespace

TEST(GameWorldDestroyQueueTest, DestroyDuringUpdateIsDeferredUntilFlush) {
    Spark::GameWorld world;
    Spark::GameObject* victim = world.CreateGameObject();
    const Spark::EntityId victimId = victim->GetEntityId();
    victim->AddComponent<Spark::TransformComponent>();

    Spark::GameObject* runner = world.CreateGameObject();
    auto* destroyer = runner->AddComponent<DestroyOnUpdate>();
    destroyer->target = victim;

    EXPECT_EQ(world.GetGameObjectCount(), 2u);
    EXPECT_TRUE(world.IsGameObjectAlive(victim));

    Spark::FrameTiming timing{};
    world.UpdateGameObjects(timing, gTestEngineContext);

    EXPECT_EQ(world.GetGameObjectCount(), 1u);
    EXPECT_TRUE(world.IsGameObjectAlive(runner));
    EXPECT_EQ(world.TryGetGameObject(victimId), nullptr);
}

TEST(GameWorldDestroyQueueTest, TryGetGameObjectByEntityId) {
    Spark::GameWorld world;
    Spark::GameObject* object = world.CreateGameObject();
    const Spark::EntityId id = object->GetEntityId();
    EXPECT_EQ(world.TryGetGameObject(id), object);
    world.DestroyGameObjectImmediate(object);
    EXPECT_EQ(world.TryGetGameObject(id), nullptr);
}

TEST(GameWorldDestroyQueueTest, IsGameObjectAliveRespectsPendingDestroy) {
    Spark::GameWorld world;
    Spark::GameObject* object = world.CreateGameObject();
    EXPECT_TRUE(world.IsGameObjectAlive(object));
    world.DestroyGameObjectAtEndOfFrame(object);
    EXPECT_FALSE(world.IsGameObjectAlive(object));
    EXPECT_NE(object, nullptr);
    world.FlushPendingDestroyGameObjects();
    EXPECT_EQ(world.TryGetGameObject(object->GetEntityId()), nullptr);
}
