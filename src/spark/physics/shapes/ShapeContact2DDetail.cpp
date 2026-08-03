#include "spark/physics/shapes/ShapeContact2DDetail.hpp"

#include "spark/physics/Collision2D.hpp"
#include "spark/physics/shapes/BoxShape2D.hpp"
#include "spark/physics/shapes/CircleShape2D.hpp"
#include "spark/physics/shapes/ConvexPolygonShape2D.hpp"
#include "spark/physics/shapes/ShapeType2D.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Spark::ShapeContact2DDetail {

namespace {

[[nodiscard]] bool FillManifoldFromSeparation(
        const float nx,
        const float ny,
        const float penetration,
        const float pointX,
        const float pointY,
        ContactManifold2D& out) noexcept {
    if (penetration <= 0.0F) {
        return false;
    }
    out.normal = {nx, ny};
    out.point = {pointX, pointY};
    out.penetration = penetration;
    return true;
}

[[nodiscard]] bool ContactBoxBox(const BoxShape2D& a, const BoxShape2D& b, ContactManifold2D& out) noexcept {
    const CollisionAabb2& boxA = a.GetAabb();
    const CollisionAabb2& boxB = b.GetAabb();
    if (!CollisionAabb2Overlaps(boxA, boxB)) {
        return false;
    }

    const float overlapX = (std::min)(boxA.maxX, boxB.maxX) - (std::max)(boxA.minX, boxB.minX);
    const float overlapY = (std::min)(boxA.maxY, boxB.maxY) - (std::max)(boxA.minY, boxB.minY);
    const float centerAx = 0.5F * (boxA.minX + boxA.maxX);
    const float centerAy = 0.5F * (boxA.minY + boxA.maxY);
    const float centerBx = 0.5F * (boxB.minX + boxB.maxX);
    const float centerBy = 0.5F * (boxB.minY + boxB.maxY);

    float nx = 0.0F;
    float ny = 0.0F;
    float pen = 0.0F;
    if (overlapX < overlapY) {
        pen = overlapX;
        nx = (centerAx < centerBx) ? -1.0F : 1.0F;
    } else {
        pen = overlapY;
        ny = (centerAy < centerBy) ? -1.0F : 1.0F;
    }
    return FillManifoldFromSeparation(nx, ny, pen, 0.5F * (centerAx + centerBx), 0.5F * (centerAy + centerBy), out);
}

[[nodiscard]] bool ContactCircleCircle(
        const CircleShape2D& a,
        const CircleShape2D& b,
        ContactManifold2D& out) noexcept {
    const float dx = b.GetCenterX() - a.GetCenterX();
    const float dy = b.GetCenterY() - a.GetCenterY();
    const float distSq = dx * dx + dy * dy;
    const float sum = a.GetRadius() + b.GetRadius();
    if (distSq > sum * sum + 1.0e-8F) {
        return false;
    }
    if (distSq <= 1.0e-12F) {
        return FillManifoldFromSeparation(1.0F, 0.0F, sum, a.GetCenterX(), a.GetCenterY(), out);
    }
    const float dist = std::sqrt(distSq);
    const float pen = sum - dist;
    return FillManifoldFromSeparation(dx / dist, dy / dist, pen, a.GetCenterX() + dx * 0.5F, a.GetCenterY() + dy * 0.5F, out);
}

[[nodiscard]] bool ContactBoxCircle(
        const BoxShape2D& box,
        const CircleShape2D& circle,
        ContactManifold2D& out) noexcept {
    const CollisionAabb2& aabb = box.GetAabb();
    const float cx = circle.GetCenterX();
    const float cy = circle.GetCenterY();
    const float cr = circle.GetRadius();
    if (!CollisionAabb2OverlapsCircle(aabb, cx, cy, cr)) {
        return false;
    }

    const float qx = std::clamp(cx, aabb.minX, aabb.maxX);
    const float qy = std::clamp(cy, aabb.minY, aabb.maxY);
    const float dx = cx - qx;
    const float dy = cy - qy;
    const float d2 = dx * dx + dy * dy;
    if (d2 > 1.0e-12F) {
        const float d = std::sqrt(d2);
        return FillManifoldFromSeparation(dx / d, dy / d, cr - d, qx, qy, out);
    }

    const float centerX = 0.5F * (aabb.minX + aabb.maxX);
    const float centerY = 0.5F * (aabb.minY + aabb.maxY);
    const float leftPen = (aabb.minX + cr) - cx;
    const float rightPen = cx - (aabb.maxX - cr);
    const float bottomPen = (aabb.minY + cr) - cy;
    const float topPen = cy - (aabb.maxY - cr);
    float bestPen = leftPen;
    float nx = -1.0F;
    float ny = 0.0F;
    if (rightPen < bestPen) {
        bestPen = rightPen;
        nx = 1.0F;
        ny = 0.0F;
    }
    if (bottomPen < bestPen) {
        bestPen = bottomPen;
        nx = 0.0F;
        ny = -1.0F;
    }
    if (topPen < bestPen) {
        bestPen = topPen;
        nx = 0.0F;
        ny = 1.0F;
    }
    return FillManifoldFromSeparation(nx, ny, bestPen, centerX, centerY, out);
}

[[nodiscard]] const BoxShape2D* AsBox(const IShape2D& shape) noexcept {
    return shape.GetType() == ShapeType2D::Box ? static_cast<const BoxShape2D*>(&shape) : nullptr;
}

[[nodiscard]] const CircleShape2D* AsCircle(const IShape2D& shape) noexcept {
    return shape.GetType() == ShapeType2D::Circle ? static_cast<const CircleShape2D*>(&shape) : nullptr;
}

[[nodiscard]] const ConvexPolygonShape2D* AsPolygon(const IShape2D& shape) noexcept {
    return shape.GetType() == ShapeType2D::ConvexPolygon ? static_cast<const ConvexPolygonShape2D*>(&shape) : nullptr;
}

}  // namespace

bool OverlapPair(const IShape2D& a, const IShape2D& b) noexcept {
    if (const BoxShape2D* boxA = AsBox(a)) {
        if (const BoxShape2D* boxB = AsBox(b)) {
            return CollisionAabb2Overlaps(boxA->GetAabb(), boxB->GetAabb());
        }
        if (const CircleShape2D* circleB = AsCircle(b)) {
            return boxA->OverlapsCircle(circleB->GetCenterX(), circleB->GetCenterY(), circleB->GetRadius());
        }
        if (const ConvexPolygonShape2D* polyB = AsPolygon(b)) {
            return CollisionConvexPolygonOverlapsWorldAabb(polyB->AsStaticColliderSnapshot(), boxA->GetAabb());
        }
    }
    if (const CircleShape2D* circleA = AsCircle(a)) {
        if (const BoxShape2D* boxB = AsBox(b)) {
            return boxB->OverlapsCircle(circleA->GetCenterX(), circleA->GetCenterY(), circleA->GetRadius());
        }
        if (const CircleShape2D* circleB = AsCircle(b)) {
            return circleA->OverlapsCircle(circleB->GetCenterX(), circleB->GetCenterY(), circleB->GetRadius());
        }
        if (const ConvexPolygonShape2D* polyB = AsPolygon(b)) {
            return CollisionConvexPolygonOverlapsWorldCircle(
                    polyB->AsStaticColliderSnapshot(), circleA->GetCenterX(), circleA->GetCenterY(), circleA->GetRadius());
        }
    }
    if (const ConvexPolygonShape2D* polyA = AsPolygon(a)) {
        const StaticCollider2D& snap = polyA->AsStaticColliderSnapshot();
        if (const BoxShape2D* boxB = AsBox(b)) {
            return CollisionConvexPolygonOverlapsWorldAabb(snap, boxB->GetAabb());
        }
        if (const CircleShape2D* circleB = AsCircle(b)) {
            return CollisionConvexPolygonOverlapsWorldCircle(
                    snap, circleB->GetCenterX(), circleB->GetCenterY(), circleB->GetRadius());
        }
        if (const ConvexPolygonShape2D* polyB = AsPolygon(b)) {
            return CollisionConvexPolygonOverlapsWorldAabb(snap, polyB->GetBounds());
        }
    }
    return false;
}

bool ContactPair(const IShape2D& a, const IShape2D& b, ContactManifold2D& out) noexcept {
    out.Clear();
    if (const BoxShape2D* boxA = AsBox(a)) {
        if (const BoxShape2D* boxB = AsBox(b)) {
            return ContactBoxBox(*boxA, *boxB, out);
        }
        if (const CircleShape2D* circleB = AsCircle(b)) {
            return ContactBoxCircle(*boxA, *circleB, out);
        }
        if (const ConvexPolygonShape2D* polyB = AsPolygon(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float pen = 0.0F;
            if (!TryComputeBoxPolygonSeparation(boxA->GetAabb(), polyB->AsStaticColliderSnapshot(), nx, ny, pen)) {
                return false;
            }
            return FillManifoldFromSeparation(nx, ny, pen, 0.0F, 0.0F, out);
        }
    }
    if (const CircleShape2D* circleA = AsCircle(a)) {
        if (const BoxShape2D* boxB = AsBox(b)) {
            ContactManifold2D flipped{};
            if (!ContactBoxCircle(*boxB, *circleA, flipped)) {
                return false;
            }
            out.normal = {-flipped.normal.x, -flipped.normal.y};
            out.point = flipped.point;
            out.penetration = flipped.penetration;
            return true;
        }
        if (const CircleShape2D* circleB = AsCircle(b)) {
            return ContactCircleCircle(*circleA, *circleB, out);
        }
        if (const ConvexPolygonShape2D* polyB = AsPolygon(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float pen = 0.0F;
            if (!TryComputeCirclePolygonSeparation(
                        circleA->GetCenterX(),
                        circleA->GetCenterY(),
                        circleA->GetRadius(),
                        polyB->AsStaticColliderSnapshot(),
                        nx,
                        ny,
                        pen)) {
                return false;
            }
            return FillManifoldFromSeparation(nx, ny, pen, circleA->GetCenterX(), circleA->GetCenterY(), out);
        }
    }
    if (const ConvexPolygonShape2D* polyA = AsPolygon(a)) {
        if (const BoxShape2D* boxB = AsBox(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float pen = 0.0F;
            if (!TryComputeBoxPolygonSeparation(boxB->GetAabb(), polyA->AsStaticColliderSnapshot(), nx, ny, pen)) {
                return false;
            }
            out.normal = {-nx, -ny};
            out.point = {0.0F, 0.0F};
            out.penetration = pen;
            return true;
        }
        if (const CircleShape2D* circleB = AsCircle(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float pen = 0.0F;
            if (!TryComputeCirclePolygonSeparation(
                        circleB->GetCenterX(),
                        circleB->GetCenterY(),
                        circleB->GetRadius(),
                        polyA->AsStaticColliderSnapshot(),
                        nx,
                        ny,
                        pen)) {
                return false;
            }
            out.normal = {-nx, -ny};
            out.point = {circleB->GetCenterX(), circleB->GetCenterY()};
            out.penetration = pen;
            return true;
        }
    }
    return false;
}

}  // namespace Spark::ShapeContact2DDetail
