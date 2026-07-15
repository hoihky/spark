#include "spark/scene/GameWorld.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/text/Font.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/core/Utility.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"
#include "spark/animation/Skeleton.hpp"

#include <algorithm>
#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] bool PathEndsWithInsensitive(const char* path, const char* suffix) {
    if (path == nullptr || suffix == nullptr) {
        return false;
    }
    const std::size_t lp = std::strlen(path);
    const std::size_t ls = std::strlen(suffix);
    if (lp < ls) {
        return false;
    }
    for (std::size_t i = 0; i < ls; ++i) {
        char a = path[lp - ls + i];
        char b = suffix[i];
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b - 'A' + 'a');
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool IsGltfPath(const char* path) {
    return PathEndsWithInsensitive(path, ".glb") || PathEndsWithInsensitive(path, ".gltf");
}

}  // namespace

GameWorld::~GameWorld() {
    Array<GameObject*> roots;
    roots.Reserve(objects.GetSize());
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        GameObject* o = objects[i].Get();
        if (o != nullptr && o->GetParent() == nullptr) {
            roots.PushBack(o);
        }
    }
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        DestroyGameObject(roots[i]);
    }
}

GameObject* GameWorld::CreateGameObject() {
    const std::uint64_t oid = nextObjectId++;
    objects.PushBack(UniquePtr<GameObject>(new GameObject(*this, oid)));
    return objects.GetLast().Get();
}

void GameWorld::CollectSubtreePostOrder(GameObject* root, Array<GameObject*>& out) {
    if (root == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < root->GetChildren().GetSize(); ++i) {
        CollectSubtreePostOrder(root->GetChildren()[i], out);
    }
    out.PushBack(root);
}

void GameWorld::DestroyGameObject(GameObject* object) {
    if (object == nullptr || object->world != this) {
        return;
    }
    Array<GameObject*> order;
    CollectSubtreePostOrder(object, order);
    for (std::size_t i = 0; i < order.GetSize(); ++i) {
        GameObject* n = order[i];
        if (n->parent != nullptr) {
            n->parent->InternalRemoveChild(n);
        }
        n->parent = nullptr;
        n->children.Clear();
        EraseGameObjectStorage(n);
    }
}

void GameWorld::EraseGameObjectStorage(GameObject* pointer) {
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        if (objects[i].Get() == pointer) {
            objects.RemoveAt(i);
            return;
        }
    }
}

bool GameWorld::SetParent(GameObject* child, GameObject* newParent) {
    if (child == nullptr || child->world != this) {
        return false;
    }
    if (newParent != nullptr) {
        if (newParent->world != this) {
            return false;
        }
        if (newParent == child) {
            return false;
        }
        for (const GameObject* p = newParent; p != nullptr; p = p->parent) {
            if (p == child) {
                return false;
            }
        }
    }
    if (child->parent != nullptr) {
        child->parent->InternalRemoveChild(child);
    }
    child->parent = newParent;
    if (newParent != nullptr) {
        newParent->InternalAddChild(child);
    }
    return true;
}

void GameWorld::UpdateGameObjects(const FrameTiming& timing, IEngineContext& context) {
    for (std::size_t i = 0; i < objects.GetSize(); ++i) {
        GameObject* o = objects[i].Get();
        if (o != nullptr) {
            o->UpdateComponents(timing, context);
        }
    }
}

SharedPtr<Mesh> GameWorld::LoadMesh(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return SharedPtr<Mesh>();
    }
    if (IsGltfPath(path)) {
        return LoadGltf(path).mesh;
    }
    const Utf8String key(path);
    if (const SharedPtr<Mesh>* cached = meshCache.Find(key)) {
        return *cached;
    }
    auto mesh = MakeShared<Mesh>(Utf8String(path));
    if (!Mesh::TryLoadFromObj(path, *mesh)) {
        return SharedPtr<Mesh>();
    }
    meshCache.Add(key, mesh);
    return mesh;
}

GltfAsset GameWorld::LoadGltf(const char* path) {
    GltfAsset empty{};
    if (path == nullptr || path[0] == '\0') {
        return empty;
    }
    const Utf8String key(path);
    if (const GltfAsset* cached = gltfCache.Find(key)) {
        return *cached;
    }
    auto mesh = MakeShared<Mesh>(Utf8String(path));
    SharedPtr<Texture2D> tex;
    if (!Mesh::TryLoadFromGltf(path, *mesh, &tex)) {
        return empty;
    }
    GltfAsset asset{};
    asset.mesh = mesh;
    asset.baseColorTexture = tex;
    gltfCache.Add(key, asset);
    return asset;
}

void GameWorld::RegisterGltf(const GltfAsset& asset, const char* cacheKey) {
    if (!asset.mesh || cacheKey == nullptr || cacheKey[0] == '\0') {
        return;
    }
    gltfCache.Add(Utf8String(cacheKey), asset);
    if (asset.baseColorTexture) {
        textureCache.Add(Utf8String(cacheKey), asset.baseColorTexture);
    }
}

SkinnedGltfAsset GameWorld::LoadSkinnedGltf(const char* path) {
    SkinnedGltfAsset empty{};
    if (path == nullptr || path[0] == '\0') {
        return empty;
    }
    const Utf8String key(path);
    if (const SkinnedGltfAsset* cached = skinnedGltfCache.Find(key)) {
        return *cached;
    }
    SkinnedMesh mesh;
    Skeleton skeleton;
    SharedPtr<Texture2D> tex;
    std::uint32_t walkClip = 0;
    Quaternion bindUp{};
    float facingYaw = 0.0F;
    if (!TryLoadSkinnedCharacterFromGltf(path, mesh, skeleton, &tex, &walkClip, &bindUp, &facingYaw)) {
        return empty;
    }
    SkinnedGltfAsset asset{};
    asset.mesh = SharedPtr<SkinnedMesh>(new SkinnedMesh(MoveTemp(mesh)));
    asset.skeleton = SharedPtr<Skeleton>(new Skeleton(MoveTemp(skeleton)));
    asset.baseColorTexture = tex;
    asset.walkClipIndex = walkClip;
    asset.bindUpAlignment = bindUp;
    asset.bindFacingYawOffset = facingYaw;
    skinnedGltfCache.Add(key, asset);
    return asset;
}

void GameWorld::RegisterSkinnedGltf(const SkinnedGltfAsset& asset, const char* cacheKey) {
    if (!asset.mesh || !asset.skeleton || cacheKey == nullptr || cacheKey[0] == '\0') {
        return;
    }
    skinnedGltfCache.Add(Utf8String(cacheKey), asset);
    if (asset.baseColorTexture) {
        textureCache.Add(Utf8String(cacheKey), asset.baseColorTexture);
    }
}

SharedPtr<Texture2D> GameWorld::LoadTexture(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return SharedPtr<Texture2D>();
    }
    const Utf8String key(path);
    if (const SharedPtr<Texture2D>* cached = textureCache.Find(key)) {
        return *cached;
    }
    auto tex = MakeShared<Texture2D>(Utf8String(path));
    if (!Texture2D::TryLoadFromFile(path, *tex)) {
        return SharedPtr<Texture2D>();
    }
    textureCache.Add(key, tex);
    return tex;
}

SharedPtr<Mesh> GameWorld::RegisterMesh(const SharedPtr<Mesh>& mesh, const char* cacheKey) {
    if (!mesh) {
        return SharedPtr<Mesh>();
    }
    if (cacheKey != nullptr && cacheKey[0] != '\0') {
        meshCache.Add(Utf8String(cacheKey), mesh);
    }
    return mesh;
}

SharedPtr<Texture2D> GameWorld::RegisterTexture(const SharedPtr<Texture2D>& texture, const char* cacheKey) {
    if (!texture) {
        return SharedPtr<Texture2D>();
    }
    if (cacheKey != nullptr && cacheKey[0] != '\0') {
        textureCache.Add(Utf8String(cacheKey), texture);
    }
    return texture;
}

SharedPtr<Mesh> GameWorld::TryGetMeshByKeyOrPath(const char* keyOrPath) const {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return SharedPtr<Mesh>();
    }
    const Utf8String key(keyOrPath);
    if (const GltfAsset* g = gltfCache.Find(key)) {
        return g->mesh;
    }
    if (const SharedPtr<Mesh>* m = meshCache.Find(key)) {
        return *m;
    }
    return SharedPtr<Mesh>();
}

SharedPtr<Texture2D> GameWorld::TryGetTextureByKeyOrPath(const char* keyOrPath) const {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return SharedPtr<Texture2D>();
    }
    const Utf8String key(keyOrPath);
    const SkinnedGltfAsset* sk = skinnedGltfCache.Find(key);
    const GltfAsset* gl = gltfCache.Find(key);
    // Skinned glTF first when both caches have the same path: rigid TryLoadFromGltf can populate gltfCache
    // with a mesh but miss the base-color texture, which would otherwise hide the skinned asset's texture.
    if (sk != nullptr && sk->baseColorTexture) {
        return sk->baseColorTexture;
    }
    if (gl != nullptr && gl->baseColorTexture) {
        return gl->baseColorTexture;
    }
    if (const SharedPtr<Texture2D>* t = textureCache.Find(key)) {
        return *t;
    }
    if (sk != nullptr) {
        return sk->baseColorTexture;
    }
    if (gl != nullptr) {
        return gl->baseColorTexture;
    }
    return SharedPtr<Texture2D>();
}

bool GameWorld::TryGetCachedSkinnedGltf(const char* path, SkinnedGltfAsset& out) const {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (const SkinnedGltfAsset* s = skinnedGltfCache.Find(Utf8String(path))) {
        out = *s;
        return static_cast<bool>(out.mesh && out.skeleton);
    }
    return false;
}

bool GameWorld::TryGetAxisAlignedBoundsForKeyOrPath(const char* keyOrPath, Vector3& outMin, Vector3& outMax)
        const {
    if (keyOrPath == nullptr || keyOrPath[0] == '\0') {
        return false;
    }
    const Utf8String key(keyOrPath);
    if (const GltfAsset* g = gltfCache.Find(key)) {
        if (g->mesh && g->mesh->TryComputeAxisAlignedBounds(outMin, outMax)) {
            return true;
        }
    }
    if (const SharedPtr<Mesh>* m = meshCache.Find(key)) {
        if (*m && (*m)->TryComputeAxisAlignedBounds(outMin, outMax)) {
            return true;
        }
    }
    if (const SkinnedGltfAsset* sk = skinnedGltfCache.Find(key)) {
        if (!sk->mesh) {
            return false;
        }
        const Array<SkinnedMesh::Vertex>& sv = sk->mesh->GetVertices();
        if (sv.IsEmpty()) {
            return false;
        }
        outMin = sv[0].position;
        outMax = sv[0].position;
        for (std::size_t vi = 1; vi < sv.GetSize(); ++vi) {
            const Vector3& p = sv[vi].position;
            outMin.x = std::min(outMin.x, p.x);
            outMin.y = std::min(outMin.y, p.y);
            outMin.z = std::min(outMin.z, p.z);
            outMax.x = std::max(outMax.x, p.x);
            outMax.y = std::max(outMax.y, p.y);
            outMax.z = std::max(outMax.z, p.z);
        }
        return true;
    }
    return false;
}

void GameWorld::SetUiFont(SharedPtr<Font> font) {
    uiFont = MoveTemp(font);
}

void GameWorld::SetUiBoldFont(SharedPtr<Font> font) {
    uiBoldFont = MoveTemp(font);
}

}  // namespace Spark
