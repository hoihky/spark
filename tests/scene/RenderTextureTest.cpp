#include <gtest/gtest.h>

#include "spark/scene/render/RenderTexture.hpp"

TEST(RenderTextureTest, ResizeBumpsGpuAllocationToken) {
    Spark::RenderTexture texture{{.width = 64, .height = 48}};
    const std::uint64_t initialToken = texture.GetGpuAllocationToken();
    texture.Resize(128, 96);
    EXPECT_NE(texture.GetGpuAllocationToken(), initialToken);
    EXPECT_EQ(texture.GetWidth(), 128U);
    EXPECT_EQ(texture.GetHeight(), 96U);
}

TEST(RenderTextureTest, SetDescBumpsTokenWhenExtentChanges) {
    Spark::RenderTexture texture{{.width = 32, .height = 32}};
    const std::uint64_t initialToken = texture.GetGpuAllocationToken();
    Spark::RenderTextureDesc next{};
    next.width = 64;
    next.height = 32;
    texture.SetDesc(next);
    EXPECT_NE(texture.GetGpuAllocationToken(), initialToken);
}

TEST(RenderTextureTest, ValidExtentBounds) {
    EXPECT_TRUE(Spark::RenderTexture::IsValidExtent(1, 1));
    EXPECT_TRUE(Spark::RenderTexture::IsValidExtent(1920, 1080));
    EXPECT_FALSE(Spark::RenderTexture::IsValidExtent(0, 64));
    EXPECT_FALSE(Spark::RenderTexture::IsValidExtent(64, 0));
}
