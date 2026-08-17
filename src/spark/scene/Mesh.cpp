#include "spark/scene/Mesh.hpp"

#include "spark/math/Constants.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace Spark {

Mesh::Mesh(Utf8String meshName) : name(MoveTemp(meshName)) {}

bool Mesh::TryComputeAxisAlignedBounds(Vector3& outMin, Vector3& outMax) const noexcept {
    if (vertices.IsEmpty()) {
        return false;
    }
    outMin = vertices[0].position;
    outMax = vertices[0].position;
    for (std::size_t i = 1; i < vertices.GetSize(); ++i) {
        const Vector3& p = vertices[i].position;
        outMin.x = std::min(outMin.x, p.x);
        outMin.y = std::min(outMin.y, p.y);
        outMin.z = std::min(outMin.z, p.z);
        outMax.x = std::max(outMax.x, p.x);
        outMax.y = std::max(outMax.y, p.y);
        outMax.z = std::max(outMax.z, p.z);
    }
    return true;
}

void Mesh::Clear() noexcept {
    vertices.Clear();
    indices.Clear();
    submeshes.Clear();
}

void Mesh::AddVertex(const Vertex& v) {
    vertices.PushBack(v);
}

void Mesh::AddTriangle(std::uint32_t i0, std::uint32_t i1, std::uint32_t i2) {
    indices.PushBack(i0);
    indices.PushBack(i1);
    indices.PushBack(i2);
}

Mesh Mesh::CreateTriangle() {
    Mesh m;
    m.vertices.PushBack({{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}});
    m.vertices.PushBack({{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}});
    m.vertices.PushBack({{0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}});
    m.indices.PushBack(0);
    m.indices.PushBack(1);
    m.indices.PushBack(2);
    return m;
}

Mesh Mesh::CreateQuad(float width, float height) {
    const float hx = width * 0.5F;
    const float hy = height * 0.5F;
    Mesh m;
    m.vertices.PushBack({{-hx, -hy, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}});
    m.vertices.PushBack({{hx, -hy, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}});
    m.vertices.PushBack({{hx, hy, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}});
    m.vertices.PushBack({{-hx, hy, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}});
    m.indices.PushBack(0);
    m.indices.PushBack(1);
    m.indices.PushBack(2);
    m.indices.PushBack(0);
    m.indices.PushBack(2);
    m.indices.PushBack(3);
    return m;
}

Mesh Mesh::CreateGroundPlane(float halfExtent, float worldUnitsPerTextureRepeat) {
    Mesh m(Utf8String("GroundPlane"));
    const float h = halfExtent;
    const float uvSpan = (2.0F * h) /
            ((worldUnitsPerTextureRepeat > 0.0F) ? worldUnitsPerTextureRepeat : (std::max)(2.0F * h, 1.0e-3F));
    m.AddVertex({{-h, 0.0F, -h}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}});
    m.AddVertex({{h, 0.0F, -h}, {0.0F, 1.0F, 0.0F}, {uvSpan, 0.0F}});
    m.AddVertex({{h, 0.0F, h}, {0.0F, 1.0F, 0.0F}, {uvSpan, uvSpan}});
    m.AddVertex({{-h, 0.0F, h}, {0.0F, 1.0F, 0.0F}, {0.0F, uvSpan}});
    // CCW from +Y; normals are fixed +Y. Ground draws disable back-face culling (GroundPlane slot).
    m.AddTriangle(0, 1, 2);
    m.AddTriangle(0, 3, 2);
    return m;
}

Mesh Mesh::CreateSimpleCar() {
    Mesh m(Utf8String("SimpleCar"));
    const float hx = 1.75F;
    const float hy = 0.55F;
    const float hz = 0.7F;
    const Vector3 p[8] = {
            {-hx, 0.0F, -hz},
            {hx, 0.0F, -hz},
            {hx, hy, -hz},
            {-hx, hy, -hz},
            {-hx, 0.0F, hz},
            {hx, 0.0F, hz},
            {hx, hy, hz},
            {-hx, hy, hz},
    };
    const Vector3 n[6] = {
            {0.0F, 0.0F, -1.0F},
            {0.0F, 0.0F, 1.0F},
            {1.0F, 0.0F, 0.0F},
            {-1.0F, 0.0F, 0.0F},
            {0.0F, 1.0F, 0.0F},
            {0.0F, -1.0F, 0.0F},
    };
    const int faces[6][4] = {
            {0, 1, 2, 3},
            {5, 4, 7, 6},
            {1, 5, 6, 2},
            {4, 0, 3, 7},
            {3, 2, 6, 7},
            {4, 5, 1, 0},
    };
    const Vector2 faceUv[4] = {
            {0.0F, 0.0F},
            {1.0F, 0.0F},
            {1.0F, 1.0F},
            {0.0F, 1.0F},
    };
    for (int f = 0; f < 6; ++f) {
        const std::uint32_t base = static_cast<std::uint32_t>(m.vertices.GetSize());
        for (int k = 0; k < 4; ++k) {
            Vertex v;
            v.position = p[faces[f][k]];
            v.normal = n[f];
            v.texCoord = faceUv[k];
            m.vertices.PushBack(v);
        }
        m.indices.PushBack(base);
        m.indices.PushBack(base + 1);
        m.indices.PushBack(base + 2);
        m.indices.PushBack(base);
        m.indices.PushBack(base + 2);
        m.indices.PushBack(base + 3);
    }
    return m;
}

Mesh Mesh::CreateUnitCube() {
    Mesh m;
    // 8 corners of [-1,1]^3
    const Vector3 p[8] = {
            {-1.0F, -1.0F, -1.0F},
            {1.0F, -1.0F, -1.0F},
            {1.0F, 1.0F, -1.0F},
            {-1.0F, 1.0F, -1.0F},
            {-1.0F, -1.0F, 1.0F},
            {1.0F, -1.0F, 1.0F},
            {1.0F, 1.0F, 1.0F},
            {-1.0F, 1.0F, 1.0F},
    };
    const Vector3 n[6] = {
            {0.0F, 0.0F, -1.0F},
            {0.0F, 0.0F, 1.0F},
            {1.0F, 0.0F, 0.0F},
            {-1.0F, 0.0F, 0.0F},
            {0.0F, 1.0F, 0.0F},
            {0.0F, -1.0F, 0.0F},
    };
    // Each face: 4 verts, 2 tris; duplicate vertices per face for correct normals
    const int faces[6][4] = {
            {0, 1, 2, 3},
            {5, 4, 7, 6},
            {1, 5, 6, 2},
            {4, 0, 3, 7},
            {3, 2, 6, 7},
            {4, 5, 1, 0},
    };
    // Per-face UVs (0,0)-(1,1) so textures tile per face; all (0,0) samples a single texel (no visible pattern).
    const Vector2 faceUv[4] = {
            {0.0F, 0.0F},
            {1.0F, 0.0F},
            {1.0F, 1.0F},
            {0.0F, 1.0F},
    };
    for (int f = 0; f < 6; ++f) {
        const std::uint32_t base = static_cast<std::uint32_t>(m.vertices.GetSize());
        for (int k = 0; k < 4; ++k) {
            Vertex v;
            v.position = p[faces[f][k]];
            v.normal = n[f];
            v.texCoord = faceUv[k];
            m.vertices.PushBack(v);
        }
        m.indices.PushBack(base);
        m.indices.PushBack(base + 1);
        m.indices.PushBack(base + 2);
        m.indices.PushBack(base);
        m.indices.PushBack(base + 2);
        m.indices.PushBack(base + 3);
    }
    return m;
}

Mesh Mesh::CreateSkyDome(float radius, int latitudeSegments, int longitudeSegments) {
    Mesh m(Utf8String("SkyDome"));
    if (latitudeSegments < 2) {
        latitudeSegments = 2;
    }
    if (longitudeSegments < 3) {
        longitudeSegments = 3;
    }
    const int latCount = latitudeSegments + 1;
    const int lonCount = longitudeSegments + 1;
    for (int iy = 0; iy < latCount; ++iy) {
        const float t = static_cast<float>(iy) / static_cast<float>(latitudeSegments);
        const float theta = t * HalfPi;
        const float sinT = std::sin(theta);
        const float cosT = std::cos(theta);
        const float y = radius * cosT;
        const float ringR = radius * sinT;
        for (int ix = 0; ix < lonCount; ++ix) {
            const float p = static_cast<float>(ix) / static_cast<float>(longitudeSegments);
            const float phi = p * TwoPi;
            const float x = ringR * std::cos(phi);
            const float z = ringR * std::sin(phi);
            const Vector3 pos{x, y, z};
            const Vector3 n = pos.Normalized();
            const Vector2 uv{p, 1.0F - t};
            m.vertices.PushBack({pos, n, uv});
        }
    }
    for (int iy = 0; iy < latitudeSegments; ++iy) {
        for (int ix = 0; ix < longitudeSegments; ++ix) {
            const std::uint32_t a = static_cast<std::uint32_t>(iy * lonCount + ix);
            const std::uint32_t b = a + 1U;
            const std::uint32_t c = a + static_cast<std::uint32_t>(lonCount);
            const std::uint32_t d = c + 1U;
            m.indices.PushBack(a);
            m.indices.PushBack(c);
            m.indices.PushBack(b);
            m.indices.PushBack(b);
            m.indices.PushBack(c);
            m.indices.PushBack(d);
        }
    }
    return m;
}

Mesh Mesh::CreateSkySphere(float radius, int latitudeSegments, int longitudeSegments) {
    Mesh m(Utf8String("SkySphere"));
    if (latitudeSegments < 2) {
        latitudeSegments = 2;
    }
    if (longitudeSegments < 3) {
        longitudeSegments = 3;
    }
    const int latCount = latitudeSegments + 1;
    const int lonCount = longitudeSegments + 1;
    for (int iy = 0; iy < latCount; ++iy) {
        const float t = static_cast<float>(iy) / static_cast<float>(latitudeSegments);
        const float theta = t * Pi;
        const float sinT = std::sin(theta);
        const float cosT = std::cos(theta);
        const float y = radius * cosT;
        const float ringR = radius * sinT;
        for (int ix = 0; ix < lonCount; ++ix) {
            const float p = static_cast<float>(ix) / static_cast<float>(longitudeSegments);
            const float phi = p * TwoPi;
            const float x = ringR * std::cos(phi);
            const float z = ringR * std::sin(phi);
            const Vector3 pos{x, y, z};
            const Vector3 n = pos.Normalized();
            const Vector2 uv{p, 1.0F - t};
            m.vertices.PushBack({pos, n, uv});
        }
    }
    for (int iy = 0; iy < latitudeSegments; ++iy) {
        for (int ix = 0; ix < longitudeSegments; ++ix) {
            const std::uint32_t a = static_cast<std::uint32_t>(iy * lonCount + ix);
            const std::uint32_t b = a + 1U;
            const std::uint32_t c = a + static_cast<std::uint32_t>(lonCount);
            const std::uint32_t d = c + 1U;
            m.indices.PushBack(a);
            m.indices.PushBack(c);
            m.indices.PushBack(b);
            m.indices.PushBack(b);
            m.indices.PushBack(c);
            m.indices.PushBack(d);
        }
    }
    return m;
}

Mesh Mesh::CreateSkyBillboardPlane(float halfWidth, float halfHeight) {
    Mesh m(Utf8String("SkyPlane"));
    m.vertices.PushBack({{-halfWidth, -halfHeight, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}});
    m.vertices.PushBack({{halfWidth, -halfHeight, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}});
    m.vertices.PushBack({{halfWidth, halfHeight, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}});
    m.vertices.PushBack({{-halfWidth, halfHeight, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}});
    // CCW from +Z so front face points toward the camera when billboarding.
    m.indices.PushBack(0);
    m.indices.PushBack(1);
    m.indices.PushBack(2);
    m.indices.PushBack(0);
    m.indices.PushBack(2);
    m.indices.PushBack(3);
    return m;
}

namespace {

bool ParseFaceCorner(const char* token, int& vi, int& ti, int& ni) {
    vi = ti = ni = 0;
    if (token == nullptr || *token == '\0') {
        return false;
    }
    // v, v/vt, v//vn, v/vt/vn
    const char* slash1 = std::strchr(token, '/');
    if (slash1 == nullptr) {
        vi = std::atoi(token);
        return vi != 0;
    }
    char buf[64];
    const std::size_t len = static_cast<std::size_t>(slash1 - token);
    if (len >= sizeof(buf)) {
        return false;
    }
    std::memcpy(buf, token, len);
    buf[len] = '\0';
    vi = std::atoi(buf);
    if (vi == 0) {
        return false;
    }
    const char* p = slash1 + 1;
    if (*p == '/') {
        ++p;
        ni = std::atoi(p);
        return true;
    }
    const char* slash2 = std::strchr(p, '/');
    if (slash2 == nullptr) {
        ti = std::atoi(p);
        return true;
    }
    const std::size_t len2 = static_cast<std::size_t>(slash2 - p);
    if (len2 >= sizeof(buf)) {
        return false;
    }
    std::memcpy(buf, p, len2);
    buf[len2] = '\0';
    ti = std::atoi(buf);
    ni = std::atoi(slash2 + 1);
    return true;
}

}  // namespace

bool Mesh::TryLoadFromObj(const char* path, Mesh& outMesh) {
    if (path == nullptr) {
        return false;
    }
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    Array<Vector3> positions;
    Array<Vector3> normals;
    Array<Vector2> texCoords;
    outMesh.Clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        std::string tag;
        iss >> tag;
        if (tag == "v") {
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
            iss >> x >> y >> z;
            positions.PushBack({x, y, z});
        } else if (tag == "vn") {
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
            iss >> x >> y >> z;
            normals.PushBack({x, y, z});
        } else if (tag == "vt") {
            float u = 0.0F;
            float v = 0.0F;
            iss >> u >> v;
            texCoords.PushBack({u, v});
        } else if (tag == "f") {
            Array<int> faceV;
            Array<int> faceT;
            Array<int> faceN;
            std::string corner;
            while (iss >> corner) {
                int vi = 0;
                int ti = 0;
                int ni = 0;
                if (!ParseFaceCorner(corner.c_str(), vi, ti, ni)) {
                    continue;
                }
                faceV.PushBack(vi);
                faceT.PushBack(ti);
                faceN.PushBack(ni);
            }
            const std::size_t n = faceV.GetSize();
            if (n < 3) {
                continue;
            }
            Array<std::uint32_t> cornerIndices;
            cornerIndices.Reserve(n);
            for (std::size_t idx = 0; idx < n; ++idx) {
                const int pi = faceV[idx] - 1;
                const int ti = faceT[idx] - 1;
                const int ni = faceN[idx] - 1;
                Vertex vert{};
                if (pi >= 0 && static_cast<std::size_t>(pi) < positions.GetSize()) {
                    vert.position = positions[static_cast<std::size_t>(pi)];
                }
                if (ni >= 0 && static_cast<std::size_t>(ni) < normals.GetSize()) {
                    vert.normal = normals[static_cast<std::size_t>(ni)];
                } else {
                    vert.normal = Vector3::UnitZ;
                }
                if (ti >= 0 && static_cast<std::size_t>(ti) < texCoords.GetSize()) {
                    vert.texCoord = texCoords[static_cast<std::size_t>(ti)];
                }
                const std::uint32_t vIndex = static_cast<std::uint32_t>(outMesh.vertices.GetSize());
                outMesh.vertices.PushBack(vert);
                cornerIndices.PushBack(vIndex);
            }
            for (std::size_t k = 2; k < n; ++k) {
                outMesh.indices.PushBack(cornerIndices[0]);
                outMesh.indices.PushBack(cornerIndices[k - 1]);
                outMesh.indices.PushBack(cornerIndices[k]);
            }
        }
    }
    return !outMesh.vertices.IsEmpty();
}

}  // namespace Spark
