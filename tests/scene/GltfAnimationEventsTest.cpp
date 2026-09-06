#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/ecs/components/animation/AnimationEventReceiverComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

}  // namespace

TEST(GltfAnimationEventsTest, FoxSidecarLoadsActiveWindowEvents) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    const std::int32_t runIdx = asset.skeleton->FindClipIndexIfNameContains("run");
    ASSERT_GE(runIdx, 0);
    const Spark::Array<Spark::AnimationClipEvent>& events = asset.skeleton->GetClipEvents(
            static_cast<std::uint32_t>(runIdx));
    ASSERT_GE(events.GetSize(), 2U);

    bool haveStart = false;
    bool haveEnd = false;
    for (std::size_t i = 0; i < events.GetSize(); ++i) {
        if (events[i].name == Spark::Utf8String("active_start")) {
            haveStart = true;
        }
        if (events[i].name == Spark::Utf8String("active_end")) {
            haveEnd = true;
        }
    }
    EXPECT_TRUE(haveStart);
    EXPECT_TRUE(haveEnd);
}

TEST(GltfAnimationEventsTest, EventReceiverImportsLoadedFoxEvents) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    Spark::AnimationEventReceiverComponent receiver{};
    receiver.ImportFromSkeleton(*asset.skeleton);
    EXPECT_GE(receiver.GetMarkers().GetSize(), 2U);
}
