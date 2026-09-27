#pragma once

/** Facade include for tilemap editor / pipeline service classes (prefer over free functions). */
#include "spark/scene/tilemap/TileAnimationResolve.hpp"
#include "spark/scene/tilemap/TileAutotileBake.hpp"
#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"
#include "spark/scene/tilemap/TmxImporter.hpp"
#include "spark/scene/tilemap/TilemapBrush.hpp"
#include "spark/scene/tilemap/TilemapDerivedDataRebake.hpp"
#include "spark/scene/tilemap/TilemapDocumentApply.hpp"
#include "spark/scene/tilemap/TilemapDocumentCapture.hpp"
#include "spark/scene/tilemap/TilemapDocumentSerializer.hpp"
#include "spark/scene/tilemap/TilemapEditSession.hpp"
#include "spark/scene/tilemap/TilemapEditValidator.hpp"
#include "spark/scene/tilemap/TilemapSparkMapExporter.hpp"
#include "spark/scene/tilemap/TilemapGameplayGridBake.hpp"
#include "spark/scene/tilemap/TilemapGameplayRules.hpp"
#include "spark/scene/tilemap/TilemapObjectQuery.hpp"
#include "spark/scene/tilemap/TilemapPick.hpp"
