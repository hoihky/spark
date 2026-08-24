#!/usr/bin/env python3
"""Update #include paths after scene folder reorganization."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Longest paths first to avoid partial replacements.
INCLUDE_MAP: list[tuple[str, str]] = [
    ("spark/scene/detail/SceneSubmitDetail.hpp", "spark/scene/submit/detail/SceneSubmitDetail.hpp"),
    ("spark/scene/detail/SceneSubmitLighting.hpp", "spark/scene/submit/detail/SceneSubmitLighting.hpp"),
    ("spark/scene/assets/gltf/GltfAssetBindings.hpp", "spark/scene/assets/gltf/GltfAssetBindings.hpp"),
    ("spark/scene/assets/gltf/GltfRigidLoader.hpp", "spark/scene/assets/gltf/GltfRigidLoader.hpp"),
    ("spark/scene/GameWorldAssetLoader.hpp", "spark/scene/assets/GameWorldAssetLoader.hpp"),
    ("spark/scene/GameWorldAssetCache.hpp", "spark/scene/assets/GameWorldAssetCache.hpp"),
    ("spark/scene/AssetLoadOutcome.hpp", "spark/scene/assets/AssetLoadOutcome.hpp"),
    ("spark/scene/AssetLoadEvents.hpp", "spark/scene/assets/AssetLoadEvents.hpp"),
    ("spark/scene/CachedAssetKind.hpp", "spark/scene/assets/CachedAssetKind.hpp"),
    ("spark/scene/CharacterCameraRig.hpp", "spark/scene/camera/CharacterCameraRig.hpp"),
    ("spark/scene/TerrainGeneratorSettings.hpp", "spark/scene/mesh/TerrainGeneratorSettings.hpp"),
    ("spark/scene/TerrainMeshGenerator.hpp", "spark/scene/mesh/TerrainMeshGenerator.hpp"),
    ("spark/scene/MaterialAssetLoader.hpp", "spark/scene/material/MaterialAssetLoader.hpp"),
    ("spark/scene/ITextureBlockCompressor.hpp", "spark/scene/texture/ITextureBlockCompressor.hpp"),
    ("spark/scene/TextureBlockCompression.hpp", "spark/scene/texture/TextureBlockCompression.hpp"),
    ("spark/scene/TextureBlockCompressor.hpp", "spark/scene/texture/TextureBlockCompressor.hpp"),
    ("spark/scene/StbImageTextureLoader.hpp", "spark/scene/texture/StbImageTextureLoader.hpp"),
    ("spark/scene/SceneDrawableFrustumSink.hpp", "spark/scene/core/SceneDrawableFrustumSink.hpp"),
    ("spark/scene/ScenePartitionKind.hpp", "spark/scene/core/ScenePartitionKind.hpp"),
    ("spark/scene/SceneSpriteTileCull.hpp", "spark/scene/submit/SceneSpriteTileCull.hpp"),
    ("spark/scene/SceneTextureAtlas.hpp", "spark/scene/texture/SceneTextureAtlas.hpp"),
    ("spark/scene/SceneInstanceId.hpp", "spark/scene/core/SceneInstanceId.hpp"),
    ("spark/scene/DrawableSortResolver.hpp", "spark/scene/submit/DrawableSortResolver.hpp"),
    ("spark/scene/RenderLayerRegistry.hpp", "spark/scene/submit/RenderLayerRegistry.hpp"),
    ("spark/scene/SceneTilemapSubmit.hpp", "spark/scene/submit/SceneTilemapSubmit.hpp"),
    ("spark/scene/Ktx2TextureLoader.hpp", "spark/scene/texture/Ktx2TextureLoader.hpp"),
    ("spark/scene/GltfAssetBindings.hpp", "spark/scene/assets/gltf/GltfAssetBindings.hpp"),
    ("spark/scene/TextureLoadOptions.hpp", "spark/scene/texture/TextureLoadOptions.hpp"),
    ("spark/scene/TextureMipLevel.hpp", "spark/scene/texture/TextureMipLevel.hpp"),
    ("spark/scene/TextureMipChain.hpp", "spark/scene/texture/TextureMipChain.hpp"),
    ("spark/scene/SceneTileAtlas.hpp", "spark/scene/texture/SceneTileAtlas.hpp"),
    ("spark/scene/MaterialAsset.hpp", "spark/scene/material/MaterialAsset.hpp"),
    ("spark/scene/GltfRigidLoader.hpp", "spark/scene/assets/gltf/GltfRigidLoader.hpp"),
    ("spark/scene/DrawableSortKey.hpp", "spark/scene/submit/DrawableSortKey.hpp"),
    ("spark/scene/SceneManager.hpp", "spark/scene/core/SceneManager.hpp"),
    ("spark/scene/ITextureLoader.hpp", "spark/scene/texture/ITextureLoader.hpp"),
    ("spark/scene/MeshRaycast.hpp", "spark/scene/mesh/MeshRaycast.hpp"),
    ("spark/scene/SceneRaycast.hpp", "spark/scene/query/SceneRaycast.hpp"),
    ("spark/scene/SceneSubmit.hpp", "spark/scene/submit/SceneSubmit.hpp"),
    ("spark/scene/RenderVolumes.hpp", "spark/scene/volume/RenderVolumes.hpp"),
    ("spark/scene/VolumeRegions.hpp", "spark/scene/volume/VolumeRegions.hpp"),
    ("spark/scene/RenderLayerId.hpp", "spark/scene/submit/RenderLayerId.hpp"),
    ("spark/scene/SkinnedMesh.hpp", "spark/scene/mesh/SkinnedMesh.hpp"),
    ("spark/scene/TextureFormat.hpp", "spark/scene/texture/TextureFormat.hpp"),
    ("spark/scene/TextureLoader.hpp", "spark/scene/texture/TextureLoader.hpp"),
    ("spark/scene/GltfMaterial.hpp", "spark/scene/material/GltfMaterial.hpp"),
    ("spark/scene/MeshSubmesh.hpp", "spark/scene/mesh/MeshSubmesh.hpp"),
    ("spark/scene/GameWorld.hpp", "spark/scene/core/GameWorld.hpp"),
    ("spark/scene/Texture2D.hpp", "spark/scene/texture/Texture2D.hpp"),
    ("spark/scene/FlyCamera.hpp", "spark/scene/camera/FlyCamera.hpp"),
    ("spark/scene/Camera2D.hpp", "spark/scene/camera/Camera2D.hpp"),
    ("spark/scene/Camera.hpp", "spark/scene/camera/Camera.hpp"),
    ("spark/scene/Mesh.hpp", "spark/scene/mesh/Mesh.hpp"),
    ("spark/scene/Scene.hpp", "spark/scene/core/Scene.hpp"),
]

CMAKE_MAP: list[tuple[str, str]] = [
    ("src/spark/scene/serialization/", "src/spark/scene/serialization/"),
    ("src/spark/scene/tilemap/", "src/spark/scene/tilemap/"),
    ("src/spark/scene/detail/", "src/spark/scene/submit/detail/"),
    ("src/spark/scene/assets/gltf/", "src/spark/scene/assets/gltf/"),
    ("src/spark/scene/GameWorldAssetLoader.cpp", "src/spark/scene/assets/GameWorldAssetLoader.cpp"),
    ("src/spark/scene/GameWorldAssetCache.cpp", "src/spark/scene/assets/GameWorldAssetCache.cpp"),
    ("src/spark/scene/CharacterCameraRig.cpp", "src/spark/scene/camera/CharacterCameraRig.cpp"),
    ("src/spark/scene/TerrainMeshGenerator.cpp", "src/spark/scene/mesh/TerrainMeshGenerator.cpp"),
    ("src/spark/scene/MaterialAssetLoader.cpp", "src/spark/scene/material/MaterialAssetLoader.cpp"),
    ("src/spark/scene/skinned_mesh_gltf.cpp", "src/spark/scene/mesh/skinned_mesh_gltf.cpp"),
    ("src/spark/scene/SceneSpriteTileCull.cpp", "src/spark/scene/submit/SceneSpriteTileCull.cpp"),
    ("src/spark/scene/StbImageTextureLoader.cpp", "src/spark/scene/texture/StbImageTextureLoader.cpp"),
    ("src/spark/scene/TextureBlockCompressor.cpp", "src/spark/scene/texture/TextureBlockCompressor.cpp"),
    ("src/spark/scene/SceneDrawableFrustumSink.hpp", "src/spark/scene/core/SceneDrawableFrustumSink.hpp"),
    ("src/spark/scene/SceneSubmitDrawPartition.cpp", "src/spark/scene/submit/SceneSubmitDrawPartition.cpp"),
    ("src/spark/scene/SceneSubmitMaterial.cpp", "src/spark/scene/submit/SceneSubmitMaterial.cpp"),
    ("src/spark/scene/SceneSubmitLighting.cpp", "src/spark/scene/submit/SceneSubmitLighting.cpp"),
    ("src/spark/scene/DrawableSortResolver.cpp", "src/spark/scene/submit/DrawableSortResolver.cpp"),
    ("src/spark/scene/RenderLayerRegistry.cpp", "src/spark/scene/submit/RenderLayerRegistry.cpp"),
    ("src/spark/scene/SceneTextureAtlas.cpp", "src/spark/scene/texture/SceneTextureAtlas.cpp"),
    ("src/spark/scene/Ktx2TextureLoader.cpp", "src/spark/scene/texture/Ktx2TextureLoader.cpp"),
    ("src/spark/scene/SceneTilemapSubmit.cpp", "src/spark/scene/submit/SceneTilemapSubmit.cpp"),
    ("src/spark/scene/GltfAssetBindings.cpp", "src/spark/scene/assets/gltf/GltfAssetBindings.cpp"),
    ("src/spark/scene/MaterialAsset.cpp", "src/spark/scene/material/MaterialAsset.cpp"),
    ("src/spark/scene/SceneSpatialCull.cpp", "src/spark/scene/submit/SceneSpatialCull.cpp"),
    ("src/spark/scene/GltfRigidLoader.cpp", "src/spark/scene/assets/gltf/GltfRigidLoader.cpp"),
    ("src/spark/scene/TextureMipChain.cpp", "src/spark/scene/texture/TextureMipChain.cpp"),
    ("src/spark/scene/TextureMipLevel.cpp", "src/spark/scene/texture/TextureMipLevel.cpp"),
    ("src/spark/scene/SceneTileAtlas.cpp", "src/spark/scene/texture/SceneTileAtlas.cpp"),
    ("src/spark/scene/stb_image_impl.cpp", "src/spark/scene/texture/stb_image_impl.cpp"),
    ("src/spark/scene/TextureFormat.cpp", "src/spark/scene/texture/TextureFormat.cpp"),
    ("src/spark/scene/VolumeRegions.cpp", "src/spark/scene/volume/VolumeRegions.cpp"),
    ("src/spark/scene/RenderVolumes.cpp", "src/spark/scene/volume/RenderVolumes.cpp"),
    ("src/spark/scene/SceneManager.cpp", "src/spark/scene/core/SceneManager.cpp"),
    ("src/spark/scene/SceneSubmit.cpp", "src/spark/scene/submit/SceneSubmit.cpp"),
    ("src/spark/scene/SceneRaycast.cpp", "src/spark/scene/query/SceneRaycast.cpp"),
    ("src/spark/scene/MeshRaycast.cpp", "src/spark/scene/mesh/MeshRaycast.cpp"),
    ("src/spark/scene/GltfMaterial.cpp", "src/spark/scene/material/GltfMaterial.cpp"),
    ("src/spark/scene/SpatialMath.cpp", "src/spark/scene/query/SpatialMath.cpp"),
    ("src/spark/scene/cgltf_impl.cpp", "src/spark/scene/assets/gltf/cgltf_impl.cpp"),
    ("src/spark/scene/TextureLoader.cpp", "src/spark/scene/texture/TextureLoader.cpp"),
    ("src/spark/scene/SkinnedMesh.cpp", "src/spark/scene/mesh/SkinnedMesh.cpp"),
    ("src/spark/scene/mesh_gltf.cpp", "src/spark/scene/mesh/mesh_gltf.cpp"),
    ("src/spark/scene/Texture2D.cpp", "src/spark/scene/texture/Texture2D.cpp"),
    ("src/spark/scene/FlyCamera.cpp", "src/spark/scene/camera/FlyCamera.cpp"),
    ("src/spark/scene/Camera2D.cpp", "src/spark/scene/camera/Camera2D.cpp"),
    ("src/spark/scene/Camera.cpp", "src/spark/scene/camera/Camera.cpp"),
    ("src/spark/scene/GameWorld.cpp", "src/spark/scene/core/GameWorld.cpp"),
    ("src/spark/scene/Mesh.cpp", "src/spark/scene/mesh/Mesh.cpp"),
]

INCLUDE_MAP.sort(key=lambda pair: len(pair[0]), reverse=True)
CMAKE_MAP.sort(key=lambda pair: len(pair[0]), reverse=True)

TEXT_SUFFIXES = {".hpp", ".cpp", ".cmake", ".txt", ".md", ".inl"}


def patch_file(path: Path, replacements: list[tuple[str, str]]) -> bool:
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return False
    original = text
    for old, new in replacements:
        text = text.replace(old, new)
    if text != original:
        path.write_text(text, encoding="utf-8")
        return True
    return False


def main() -> int:
    changed = 0
    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix not in TEXT_SUFFIXES:
            continue
        if ".git" in path.parts or "cmake-build" in path.parts:
            continue
        replacements = INCLUDE_MAP
        if path.name == "CMakeLists.txt" or path.suffix == ".cmake":
            replacements = INCLUDE_MAP + CMAKE_MAP
        if patch_file(path, replacements):
            changed += 1
            print(f"updated: {path.relative_to(ROOT)}")
    print(f"Done. {changed} files updated.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
