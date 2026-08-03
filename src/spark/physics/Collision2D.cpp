#include "spark/physics/Collision2D.hpp"

#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/PolygonCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/TilemapCollider2D.hpp"
#include "spark/physics/PolygonCollider2D.hpp"
#include "spark/physics/shapes/ShapeFactory2D.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Spark {

namespace {

[[nodiscard]] Vector3 Hp3(const Vector4& p) noexcept {
    const float w = (std::fabs(p.w) < 1.0e-8F) ? 1.0F : p.w;
    return {p.x / w, p.y / w, p.z / w};
}

}  // namespace

bool CollisionAabb2Overlaps(const CollisionAabb2& a, const CollisionAabb2& b) noexcept {
    return a.minX < b.maxX && a.maxX > b.minX && a.minY < b.maxY && a.maxY > b.minY;
}

bool CollisionAabb2OverlapsCircle(const CollisionAabb2& a, const float cx, const float cy, const float r) noexcept {
    const float qx = std::clamp(cx, a.minX, a.maxX);
    const float qy = std::clamp(cy, a.minY, a.maxY);
    const float dx = cx - qx;
    const float dy = cy - qy;
    const float rr = r * r;
    const float d2 = dx * dx + dy * dy;
    return d2 <= rr + 1.0e-8F;
}

bool CollisionCirclesOverlap(const float ax, const float ay, const float ar, const float bx, const float by, const float br) noexcept {
    const float dx = ax - bx;
    const float dy = ay - by;
    const float sum = ar + br;
    return dx * dx + dy * dy <= sum * sum + 1.0e-8F;
}

bool RaycastSegmentAabb2(
        const float ox,
        const float oy,
        const float dx,
        const float dy,
        const float maxT,
        const CollisionAabb2& box,
        float& outT) noexcept {
    constexpr float kEps = 1.0e-8F;
    float t0 = 0.0F;
    float t1 = maxT;

    const auto clipAxis = [&](const float o, const float d, const float minB, const float maxB) -> bool {
        if (std::fabs(d) < kEps) {
            if (o < minB - kEps || o > maxB + kEps) {
                return false;
            }
            return true;
        }
        const float inv = 1.0F / d;
        float ta = (minB - o) * inv;
        float tb = (maxB - o) * inv;
        if (ta > tb) {
            std::swap(ta, tb);
        }
        t0 = (std::max)(t0, ta);
        t1 = (std::min)(t1, tb);
        return t0 <= t1 + kEps;
    };

    if (!clipAxis(ox, dx, box.minX, box.maxX)) {
        return false;
    }
    if (!clipAxis(oy, dy, box.minY, box.maxY)) {
        return false;
    }

    const float tHit = (std::max)(0.0F, t0);
    if (tHit > t1 + kEps || tHit > maxT + kEps) {
        return false;
    }
    outT = tHit;
    return true;
}

bool RaycastSegmentCircle2(
        const float ox,
        const float oy,
        const float dx,
        const float dy,
        const float maxT,
        const float cx,
        const float cy,
        const float r,
        float& outT) noexcept {
    constexpr float kEps = 1.0e-8F;
    const float lx = ox - cx;
    const float ly = oy - cy;
    const float a = dx * dx + dy * dy;
    if (a < kEps) {
        return false;
    }
    const float b = 2.0F * (dx * lx + dy * ly);
    const float c = lx * lx + ly * ly - r * r;
    const float disc = b * b - 4.0F * a * c;
    if (disc < 0.0F) {
        return false;
    }
    const float sd = std::sqrt(disc);
    const float inv2a = 1.0F / (2.0F * a);
    const float tA = (-b - sd) * inv2a;
    const float tB = (-b + sd) * inv2a;

    float best = std::numeric_limits<float>::infinity();
    auto consider = [&](const float t) noexcept {
        if (t >= -kEps && t <= maxT + kEps) {
            const float tt = (std::max)(0.0F, t);
            if (tt < best) {
                best = tt;
            }
        }
    };
    consider(tA);
    consider(tB);
    if (!std::isfinite(best)) {
        return false;
    }
    outT = best;
    return true;
}

bool StaticCollider2DOverlapsWorldAabb(const StaticCollider2D& s, const CollisionAabb2& w) noexcept {
    return Collider2D::FromLegacySnapshot(s).OverlapsAabb(w);
}

bool Collider2DOverlapsWorldAabb(const Collider2D& collider, const CollisionAabb2& w) noexcept {
    return collider.OverlapsAabb(w);
}

bool StaticCollider2DOverlapsWorldCircle(
        const StaticCollider2D& s, const float cx, const float cy, const float r) noexcept {
    return Collider2D::FromLegacySnapshot(s).OverlapsCircle(cx, cy, r);
}

bool Collider2DOverlapsWorldCircle(const Collider2D& collider, const float cx, const float cy, const float r) noexcept {
    return collider.OverlapsCircle(cx, cy, r);
}

void ComputeBoxCollider2WorldAabb(
        GameObject& owner,
        const BoxCollider2DComponent& collider,
        CollisionAabb2& outWorld) noexcept {
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector2 off = collider.GetOffset();
    const Vector2 he = collider.GetHalfExtents();
    const float x0 = off.x - he.x;
    const float y0 = off.y - he.y;
    const float x1 = off.x + he.x;
    const float y1 = off.y + he.y;
    const Vector4 p0 = wm * Vector4(x0, y0, 0.0F, 1.0F);
    const Vector4 p1 = wm * Vector4(x1, y0, 0.0F, 1.0F);
    const Vector4 p2 = wm * Vector4(x1, y1, 0.0F, 1.0F);
    const Vector4 p3 = wm * Vector4(x0, y1, 0.0F, 1.0F);
    const Vector3 v0 = Hp3(p0);
    const Vector3 v1 = Hp3(p1);
    const Vector3 v2 = Hp3(p2);
    const Vector3 v3 = Hp3(p3);
    outWorld.minX = (std::min)({v0.x, v1.x, v2.x, v3.x});
    outWorld.maxX = (std::max)({v0.x, v1.x, v2.x, v3.x});
    outWorld.minY = (std::min)({v0.y, v1.y, v2.y, v3.y});
    outWorld.maxY = (std::max)({v0.y, v1.y, v2.y, v3.y});
}

void ComputeCircleCollider2World(
        GameObject& owner,
        const CircleCollider2DComponent& collider,
        float& outCx,
        float& outCy,
        float& outR) noexcept {
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector2 off = collider.GetOffset();
    const Vector4 pc = wm * Vector4(off.x, off.y, 0.0F, 1.0F);
    const Vector3 c = Hp3(pc);
    outCx = c.x;
    outCy = c.y;
    // Column-major mat4: XY lengths of basis columns 0 and 1 (see Matrix4::m layout).
    const float sx = std::hypot(wm.m[0], wm.m[1]);
    const float sy = std::hypot(wm.m[4], wm.m[5]);
    const float scale = 0.5F * (sx + sy);
    outR = collider.GetRadius() * scale;
}

bool ContributesStaticCollider2D(GameObject& object) noexcept {
    if (ContributesTilemapCollider2DStatic(object)) {
        return true;
    }
    if (ContributesPolygonCollider2DStatic(object)) {
        return true;
    }
    const BoxCollider2DComponent* box = object.GetComponent<BoxCollider2DComponent>();
    const CircleCollider2DComponent* circ = object.GetComponent<CircleCollider2DComponent>();
    if (box == nullptr && circ == nullptr) {
        return false;
    }
    const Rigidbody2DComponent* rb = object.GetComponent<Rigidbody2DComponent>();
    if (rb == nullptr) {
        return true;
    }
    return rb->GetBodyType() != RigidbodyBodyType2D::Dynamic;
}

void ComputePolygonCollider2DWorld(
        GameObject& owner,
        const PolygonCollider2DComponent& collider,
        StaticCollider2D& outStatic) noexcept {
    outStatic.polygonVertexCount = 0;
    const std::uint32_t count = collider.GetVertexCount();
    if (count < 3 || count > kMaxStaticPolygonVertices) {
        return;
    }
    const Matrix4 wm = owner.GetWorldMatrix();
    float minX = 0.0F;
    float minY = 0.0F;
    float maxX = 0.0F;
    float maxY = 0.0F;
    for (std::uint32_t i = 0; i < count; ++i) {
        const Vector2& v = collider.GetVertices()[i];
        const Vector4 p = wm * Vector4(v.x, v.y, 0.0F, 1.0F);
        const Vector3 w = Hp3(p);
        outStatic.polygonVertsX[i] = w.x;
        outStatic.polygonVertsY[i] = w.y;
        if (i == 0) {
            minX = maxX = w.x;
            minY = maxY = w.y;
        } else {
            minX = (std::min)(minX, w.x);
            maxX = (std::max)(maxX, w.x);
            minY = (std::min)(minY, w.y);
            maxY = (std::max)(maxY, w.y);
        }
    }
    outStatic.polygonVertexCount = static_cast<std::uint8_t>(count);
    outStatic.aabb.minX = minX;
    outStatic.aabb.maxX = maxX;
    outStatic.aabb.minY = minY;
    outStatic.aabb.maxY = maxY;
}

namespace Collision2DDetail {

void ProjectPolygonOnAxis(
        const StaticCollider2D& poly,
        const float ax,
        const float ay,
        float& outMin,
        float& outMax) noexcept {
    outMin = outMax = poly.polygonVertsX[0] * ax + poly.polygonVertsY[0] * ay;
    for (std::uint8_t i = 1; i < poly.polygonVertexCount; ++i) {
        const float p = poly.polygonVertsX[i] * ax + poly.polygonVertsY[i] * ay;
        outMin = (std::min)(outMin, p);
        outMax = (std::max)(outMax, p);
    }
}

void ProjectAabbOnAxis(
        const CollisionAabb2& box,
        const float ax,
        const float ay,
        float& outMin,
        float& outMax) noexcept {
    const float x0 = box.minX * ax;
    const float x1 = box.maxX * ax;
    const float y0 = box.minY * ay;
    const float y1 = box.maxY * ay;
    outMin = (std::min)({x0 + y0, x0 + y1, x1 + y0, x1 + y1});
    outMax = (std::max)({x0 + y0, x0 + y1, x1 + y0, x1 + y1});
}

}  // namespace Collision2DDetail

bool CollisionConvexPolygonOverlapsWorldAabb(const StaticCollider2D& poly, const CollisionAabb2& box) noexcept {
    if (poly.polygonVertexCount < 3) {
        return false;
    }
    using namespace Collision2DDetail;
    const float axesX[] = {1.0F, 0.0F, 0.0F, 1.0F};
    const float axesY[] = {0.0F, 1.0F, 0.0F, 0.0F};
    for (int ai = 0; ai < 4; ++ai) {
        float ax = axesX[ai];
        float ay = axesY[ai];
        if (ai >= 2) {
            const std::uint8_t i = static_cast<std::uint8_t>(ai - 2);
            const std::uint8_t j = static_cast<std::uint8_t>((i + 1) % poly.polygonVertexCount);
            const float ex = poly.polygonVertsX[j] - poly.polygonVertsX[i];
            const float ey = poly.polygonVertsY[j] - poly.polygonVertsY[i];
            ax = -ey;
            ay = ex;
            const float len2 = ax * ax + ay * ay;
            if (len2 <= 1.0e-12F) {
                continue;
            }
            const float inv = 1.0F / std::sqrt(len2);
            ax *= inv;
            ay *= inv;
        }
        float pMin = 0.0F;
        float pMax = 0.0F;
        float bMin = 0.0F;
        float bMax = 0.0F;
        ProjectPolygonOnAxis(poly, ax, ay, pMin, pMax);
        ProjectAabbOnAxis(box, ax, ay, bMin, bMax);
        if (pMax < bMin || bMax < pMin) {
            return false;
        }
    }
    return true;
}

bool CollisionConvexPolygonOverlapsWorldCircle(
        const StaticCollider2D& poly,
        const float cx,
        const float cy,
        const float r) noexcept {
    if (poly.polygonVertexCount < 3) {
        return false;
    }
    float bestD2 = std::numeric_limits<float>::infinity();
    for (std::uint8_t i = 0; i < poly.polygonVertexCount; ++i) {
        const float dx = cx - poly.polygonVertsX[i];
        const float dy = cy - poly.polygonVertsY[i];
        bestD2 = (std::min)(bestD2, dx * dx + dy * dy);
    }
    for (std::uint8_t i = 0; i < poly.polygonVertexCount; ++i) {
        const std::uint8_t j = static_cast<std::uint8_t>((i + 1) % poly.polygonVertexCount);
        const float ax = poly.polygonVertsX[i];
        const float ay = poly.polygonVertsY[i];
        const float bx = poly.polygonVertsX[j];
        const float by = poly.polygonVertsY[j];
        const float ex = bx - ax;
        const float ey = by - ay;
        const float t = std::clamp(((cx - ax) * ex + (cy - ay) * ey) / std::max(ex * ex + ey * ey, 1.0e-8F), 0.0F, 1.0F);
        const float qx = ax + ex * t;
        const float qy = ay + ey * t;
        const float dx = cx - qx;
        const float dy = cy - qy;
        bestD2 = (std::min)(bestD2, dx * dx + dy * dy);
    }
    return bestD2 <= r * r;
}

bool TryComputeBoxPolygonSeparation(
        const CollisionAabb2& box,
        const StaticCollider2D& poly,
        float& outNx,
        float& outNy,
        float& outPenetration) noexcept {
    if (!CollisionConvexPolygonOverlapsWorldAabb(poly, box)) {
        return false;
    }
    float bestOverlap = std::numeric_limits<float>::infinity();
    float bestNx = 0.0F;
    float bestNy = 1.0F;
    using namespace Collision2DDetail;
    const float axesX[] = {1.0F, 0.0F};
    const float axesY[] = {0.0F, 1.0F};
    for (int ai = 0; ai < static_cast<int>(poly.polygonVertexCount) + 2; ++ai) {
        float ax = 0.0F;
        float ay = 0.0F;
        if (ai < 2) {
            ax = axesX[ai];
            ay = axesY[ai];
        } else {
            const std::uint8_t i = static_cast<std::uint8_t>(ai - 2);
            const std::uint8_t j = static_cast<std::uint8_t>((i + 1) % poly.polygonVertexCount);
            const float ex = poly.polygonVertsX[j] - poly.polygonVertsX[i];
            const float ey = poly.polygonVertsY[j] - poly.polygonVertsY[i];
            ax = -ey;
            ay = ex;
            const float len2 = ax * ax + ay * ay;
            if (len2 <= 1.0e-12F) {
                continue;
            }
            const float inv = 1.0F / std::sqrt(len2);
            ax *= inv;
            ay *= inv;
        }
        float pMin = 0.0F;
        float pMax = 0.0F;
        float bMin = 0.0F;
        float bMax = 0.0F;
        ProjectPolygonOnAxis(poly, ax, ay, pMin, pMax);
        ProjectAabbOnAxis(box, ax, ay, bMin, bMax);
        const float overlap = (std::min)(pMax, bMax) - (std::max)(pMin, bMin);
        if (overlap <= 0.0F) {
            return false;
        }
        if (overlap < bestOverlap) {
            bestOverlap = overlap;
            bestNx = ax;
            bestNy = ay;
            const float boxCenter = (box.minX + box.maxX) * 0.5F * ax + (box.minY + box.maxY) * 0.5F * ay;
            const float polyCenter = (pMin + pMax) * 0.5F;
            if (boxCenter < polyCenter) {
                bestNx = -bestNx;
                bestNy = -bestNy;
            }
        }
    }
    outNx = bestNx;
    outNy = bestNy;
    outPenetration = bestOverlap;
    return true;
}

bool TryComputeCirclePolygonSeparation(
        const float cx,
        const float cy,
        const float cr,
        const StaticCollider2D& poly,
        float& outNx,
        float& outNy,
        float& outPenetration) noexcept {
    if (!CollisionConvexPolygonOverlapsWorldCircle(poly, cx, cy, cr)) {
        return false;
    }
    float bestD2 = std::numeric_limits<float>::infinity();
    float qx = cx;
    float qy = cy;
    for (std::uint8_t i = 0; i < poly.polygonVertexCount; ++i) {
        const std::uint8_t j = static_cast<std::uint8_t>((i + 1) % poly.polygonVertexCount);
        const float ax = poly.polygonVertsX[i];
        const float ay = poly.polygonVertsY[i];
        const float bx = poly.polygonVertsX[j];
        const float by = poly.polygonVertsY[j];
        const float ex = bx - ax;
        const float ey = by - ay;
        const float t = std::clamp(((cx - ax) * ex + (cy - ay) * ey) / std::max(ex * ex + ey * ey, 1.0e-8F), 0.0F, 1.0F);
        const float px = ax + ex * t;
        const float py = ay + ey * t;
        const float dx = cx - px;
        const float dy = cy - py;
        const float d2 = dx * dx + dy * dy;
        if (d2 < bestD2) {
            bestD2 = d2;
            qx = px;
            qy = py;
        }
    }
    float dx = cx - qx;
    float dy = cy - qy;
    float d2 = dx * dx + dy * dy;
    if (d2 <= 1.0e-8F) {
        outNx = 1.0F;
        outNy = 0.0F;
        outPenetration = cr;
        return true;
    }
    const float d = std::sqrt(d2);
    outNx = dx / d;
    outNy = dy / d;
    outPenetration = cr - d;
    return outPenetration > 0.0F;
}

bool IsStaticBoxCollider2D(GameObject& object) noexcept {
    const BoxCollider2DComponent* col = object.GetComponent<BoxCollider2DComponent>();
    if (col == nullptr) {
        return false;
    }
    const Rigidbody2DComponent* rb = object.GetComponent<Rigidbody2DComponent>();
    if (rb == nullptr) {
        return true;
    }
    return rb->GetBodyType() != RigidbodyBodyType2D::Dynamic;
}

}  // namespace Spark
