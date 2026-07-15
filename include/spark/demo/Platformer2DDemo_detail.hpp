#pragma once

#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"

namespace {

constexpr std::uint32_t kPlayerAtlasRows = 1U;

using Spark::DemoAssets::KenneyPackTileNumberToSparkLinear;
using Spark::DemoAssets::KenneySimplifiedPlatformerTileUv;
using Spark::DemoAssets::kKenneyTilesheetCols;
using Spark::DemoAssets::kKenneyTilesheetRows;
using Spark::DemoAssets::kPlayerAtlasFallbackCols;
using Spark::DemoAssets::MakeGemTextureFallback;
using Spark::DemoAssets::MakePlayerRunAtlasFallback;
using Spark::DemoAssets::TryBuildKenneyPlayerAtlas;
using Spark::DemoAssets::TryLoadKenneyGemCollectible;
using Spark::DemoAssets::TryLoadKenneySimplifiedPlatformerTilesheet;

}  // namespace
