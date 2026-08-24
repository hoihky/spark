#!/usr/bin/env bash
# Reorganize spark/scene into functional subfolders.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

move_hpp() {
    local sub="$1" file="$2"
    git mv "include/spark/scene/${file}" "include/spark/scene/${sub}/${file}"
}

move_cpp() {
    local sub="$1" file="$2"
    git mv "src/spark/scene/${file}" "src/spark/scene/${sub}/${file}"
}

mkdir -p include/spark/scene/{core,camera,mesh,material,assets/gltf,texture,submit/detail,query,volume}
mkdir -p src/spark/scene/{core,camera,mesh,material,assets/gltf,texture,submit,query,volume}

# core
for f in Scene.hpp SceneManager.hpp GameWorld.hpp SceneInstanceId.hpp ScenePartitionKind.hpp SceneDrawableFrustumSink.hpp; do
    move_hpp core "$f"
done
for f in GameWorld.cpp SceneManager.cpp; do
    move_cpp core "$f"
done

# camera
for f in Camera.hpp Camera2D.hpp FlyCamera.hpp CharacterCameraRig.hpp; do
    move_hpp camera "$f"
done
for f in Camera.cpp Camera2D.cpp FlyCamera.cpp CharacterCameraRig.cpp; do
    move_cpp camera "$f"
done

# mesh
for f in Mesh.hpp MeshSubmesh.hpp SkinnedMesh.hpp MeshRaycast.hpp TerrainMeshGenerator.hpp TerrainGeneratorSettings.hpp; do
    move_hpp mesh "$f"
done
for f in Mesh.cpp SkinnedMesh.cpp MeshRaycast.cpp TerrainMeshGenerator.cpp mesh_gltf.cpp skinned_mesh_gltf.cpp; do
    move_cpp mesh "$f"
done

# material
for f in MaterialAsset.hpp MaterialAssetLoader.hpp GltfMaterial.hpp; do
    move_hpp material "$f"
done
for f in MaterialAsset.cpp MaterialAssetLoader.cpp GltfMaterial.cpp; do
    move_cpp material "$f"
done

# assets
for f in GameWorldAssetCache.hpp GameWorldAssetLoader.hpp AssetLoadEvents.hpp AssetLoadOutcome.hpp CachedAssetKind.hpp; do
    move_hpp assets "$f"
done
for f in GameWorldAssetCache.cpp GameWorldAssetLoader.cpp; do
    move_cpp assets "$f"
done

# assets/gltf
for f in GltfRigidLoader.hpp GltfAssetBindings.hpp; do
    move_hpp assets/gltf "$f"
done
for f in GltfRigidLoader.cpp GltfAssetBindings.cpp cgltf_impl.cpp; do
    move_cpp assets/gltf "$f"
done

# texture
for f in Texture2D.hpp TextureFormat.hpp TextureMipChain.hpp TextureMipLevel.hpp TextureLoader.hpp \
         ITextureLoader.hpp StbImageTextureLoader.hpp Ktx2TextureLoader.hpp TextureLoadOptions.hpp \
         TextureBlockCompressor.hpp TextureBlockCompression.hpp ITextureBlockCompressor.hpp \
         SceneTextureAtlas.hpp SceneTileAtlas.hpp; do
    move_hpp texture "$f"
done
for f in Texture2D.cpp TextureFormat.cpp TextureMipChain.cpp TextureMipLevel.cpp TextureLoader.cpp \
         StbImageTextureLoader.cpp Ktx2TextureLoader.cpp TextureBlockCompressor.cpp \
         SceneTextureAtlas.cpp SceneTileAtlas.cpp stb_image_impl.cpp; do
    move_cpp texture "$f"
done

# submit
for f in SceneSubmit.hpp SceneTilemapSubmit.hpp SceneSpriteTileCull.hpp DrawableSortResolver.hpp \
         DrawableSortKey.hpp RenderLayerRegistry.hpp RenderLayerId.hpp; do
    move_hpp submit "$f"
done
for f in SceneSubmitDetail.hpp SceneSubmitLighting.hpp; do
    git mv "include/spark/scene/detail/${f}" "include/spark/scene/submit/detail/${f}"
done
for f in SceneSubmit.cpp SceneSubmitMaterial.cpp SceneSubmitLighting.cpp SceneSubmitDrawPartition.cpp \
         SceneTilemapSubmit.cpp SceneSpatialCull.cpp SceneSpriteTileCull.cpp DrawableSortResolver.cpp \
         RenderLayerRegistry.cpp; do
    move_cpp submit "$f"
done

# query
move_hpp query SceneRaycast.hpp
for f in SceneRaycast.cpp SpatialMath.cpp; do
    move_cpp query "$f"
done

# volume
for f in RenderVolumes.hpp VolumeRegions.hpp; do
    move_hpp volume "$f"
done
for f in RenderVolumes.cpp VolumeRegions.cpp; do
    move_cpp volume "$f"
done

# remove empty detail folder if present
rmdir include/spark/scene/detail 2>/dev/null || true

echo "Scene file moves complete."
