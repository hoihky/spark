#include "spark/physics/Collision2D.hpp"

#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
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
    if (s.shape == StaticCollider2DShape::Box) {
        return CollisionAabb2Overlaps(s.aabb, w);
    }
    return CollisionAabb2OverlapsCircle(w, s.circleCx, s.circleCy, s.circleR);
}

bool StaticCollider2DOverlapsWorldCircle(
        const StaticCollider2D& s, const float cx, const float cy, const float r) noexcept {
    if (s.shape == StaticCollider2DShape::Box) {
        return CollisionAabb2OverlapsCircle(s.aabb, cx, cy, r);
    }
    return CollisionCirclesOverlap(s.circleCx, s.circleCy, s.circleR, cx, cy, r);
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
