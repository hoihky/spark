#include "spark/physics/simulation/ContactResolver2D.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/CollisionFilter2D.hpp"
#include "spark/physics/colliders/DynamicBody2D.hpp"
#include "spark/physics/colliders/DynamicCollider2D.hpp"
#include "spark/physics/core/ColliderMaterial.hpp"
#include "spark/physics/PhysicsMaterial2D.hpp"
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/physics/simulation/TriggerDispatcher2D.hpp"
#include "spark/physics/shapes/ShapeType2D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace ContactResolver2DDetail {

[[nodiscard]] bool OverlapDynamicBoxWithStatic(
        GameObject& dyn,
        const BoxCollider2DComponent& col,
        const Collider2D& st) noexcept {
    CollisionAabb2 box{};
    ComputeBoxCollider2WorldAabb(dyn, col, box);
    return st.OverlapsAabb(box);
}

[[nodiscard]] bool OverlapDynamicCircleWithStatic(
        GameObject& dyn,
        const CircleCollider2DComponent& col,
        const Collider2D& st) noexcept {
    float cx = 0.0F;
    float cy = 0.0F;
    float cr = 0.0F;
    ComputeCircleCollider2World(dyn, col, cx, cy, cr);
    return st.OverlapsCircle(cx, cy, cr);
}

constexpr float kDefaultDynamicRestitution2D = 0.12F;

void ResolveNormalVelocity(Vector2& v, const float nx, const float ny, const float restitution) noexcept {
    const float vn = v.x * nx + v.y * ny;
    if (vn < 0.0F) {
        const float bounce = 1.0F + std::clamp(restitution, 0.0F, 1.0F);
        v.x -= bounce * vn * nx;
        v.y -= bounce * vn * ny;
    }
}

[[nodiscard]] float RestitutionForStaticContact(const Collider2D& col) noexcept {
    const ColliderMaterial& mat = col.GetMaterial();
    if (!mat.isDefined) {
        return 0.0F;
    }
    return CombineRestitution2D(kDefaultDynamicRestitution2D, mat.restitution);
}

[[nodiscard]] float RestitutionForStaticContact(const StaticCollider2D& poly) noexcept {
    const ColliderMaterial mat = ColliderMaterial::FromStaticCollider2D(poly);
    if (!mat.isDefined) {
        return 0.0F;
    }
    return CombineRestitution2D(kDefaultDynamicRestitution2D, mat.restitution);
}

void ZeroVelocityIntoNormal(Vector2& v, const float nx, const float ny) noexcept {
    ResolveNormalVelocity(v, nx, ny, 0.0F);
}

[[nodiscard]] bool TryResolveBoxVsStaticAabb(
        CollisionAabb2& box,
        TransformComponent& tr,
        Rigidbody2DComponent& rb,
        const CollisionAabb2& s,
        const float restitution) noexcept {
    if (!CollisionAabb2Overlaps(box, s)) {
        return false;
    }
    const float penL = box.maxX - s.minX;
    const float penR = s.maxX - box.minX;
    const float penD = box.maxY - s.minY;
    const float penU = s.maxY - box.minY;
    int axis = 0;
    float m = penL;
    if (penR < m) {
        m = penR;
        axis = 1;
    }
    if (penD < m) {
        m = penD;
        axis = 2;
    }
    if (penU < m) {
        axis = 3;
    }
    Vector3 pos = tr.GetLocalTransform().translation;
    Vector2 v = rb.GetVelocity();
    if (axis == 3) {
        pos.y += penU;
        ResolveNormalVelocity(v, 0.0F, 1.0F, restitution);
        rb.SetGrounded(true);
    } else if (axis == 2) {
        pos.y -= penD;
        ResolveNormalVelocity(v, 0.0F, -1.0F, restitution);
    } else if (axis == 0) {
        pos.x -= penL;
        ResolveNormalVelocity(v, -1.0F, 0.0F, restitution);
    } else {
        pos.x += penR;
        ResolveNormalVelocity(v, 1.0F, 0.0F, restitution);
    }
    tr.SetTranslation(pos);
    rb.SetVelocity(v);
    return true;
}

[[nodiscard]] bool TryResolveBoxVsStaticCircle(
        CollisionAabb2& box,
        TransformComponent& tr,
        Rigidbody2DComponent& rb,
        const float scx,
        const float scy,
        const float sr,
        const float restitution) noexcept {
    if (!CollisionAabb2OverlapsCircle(box, scx, scy, sr)) {
        return false;
    }
    float qx = std::clamp(scx, box.minX, box.maxX);
    float qy = std::clamp(scy, box.minY, box.maxY);
    float dx = scx - qx;
    float dy = scy - qy;
    float d2 = dx * dx + dy * dy;
    Vector3 pos = tr.GetLocalTransform().translation;
    Vector2 v = rb.GetVelocity();
    constexpr float kEps = 1.0e-6F;
    if (d2 > kEps * kEps) {
        const float d = std::sqrt(d2);
        const float pen = sr - d;
        if (pen <= 0.0F) {
            return false;
        }
        const float inv = 1.0F / d;
        const float nx = (qx - scx) * inv;
        const float ny = (qy - scy) * inv;
        pos.x += nx * pen;
        pos.y += ny * pen;
        ResolveNormalVelocity(v, nx, ny, restitution);
        if (ny > 0.55F) {
            rb.SetGrounded(true);
        }
    } else {
        const float penL = scx - box.minX;
        const float penR = box.maxX - scx;
        const float penD = scy - box.minY;
        const float penU = box.maxY - scy;
        int axis = 0;
        float m = penL;
        if (penR < m) {
            m = penR;
            axis = 1;
        }
        if (penD < m) {
            m = penD;
            axis = 2;
        }
        if (penU < m) {
            axis = 3;
        }
        if (m >= sr) {
            return false;
        }
        const float pen = sr - m;
        if (axis == 0) {
            pos.x += pen;
            v.x = 0.0F;
        } else if (axis == 1) {
            pos.x -= pen;
            v.x = 0.0F;
        } else if (axis == 2) {
            pos.y += pen;
            v.y = 0.0F;
            rb.SetGrounded(true);
        } else {
            pos.y -= pen;
            v.y = 0.0F;
        }
    }
    tr.SetTranslation(pos);
    rb.SetVelocity(v);
    return true;
}

[[nodiscard]] bool TryResolveCircleVsStaticAabb(
        const float cx,
        const float cy,
        const float cr,
        TransformComponent& tr,
        Rigidbody2DComponent& rb,
        const CollisionAabb2& s,
        const float restitution) noexcept {
    if (!CollisionAabb2OverlapsCircle(s, cx, cy, cr)) {
        return false;
    }
    float qx = std::clamp(cx, s.minX, s.maxX);
    float qy = std::clamp(cy, s.minY, s.maxY);
    float dx = cx - qx;
    float dy = cy - qy;
    float d2 = dx * dx + dy * dy;
    Vector3 pos = tr.GetLocalTransform().translation;
    Vector2 v = rb.GetVelocity();
    constexpr float kEps = 1.0e-6F;
    if (d2 > kEps * kEps) {
        const float d = std::sqrt(d2);
        const float pen = cr - d;
        if (pen <= 0.0F) {
            return false;
        }
        const float inv = 1.0F / d;
        const float nx = dx * inv;
        const float ny = dy * inv;
        pos.x += nx * pen;
        pos.y += ny * pen;
        ResolveNormalVelocity(v, nx, ny, restitution);
        if (ny > 0.55F) {
            rb.SetGrounded(true);
        }
    } else {
        const float penL = cx - s.minX;
        const float penR = s.maxX - cx;
        const float penD = cy - s.minY;
        const float penU = s.maxY - cy;
        int axis = 0;
        float m = penL;
        if (penR < m) {
            m = penR;
            axis = 1;
        }
        if (penD < m) {
            m = penD;
            axis = 2;
        }
        if (penU < m) {
            axis = 3;
        }
        if (m >= cr) {
            return false;
        }
        const float pen = cr - m;
        if (axis == 0) {
            pos.x += pen;
            v.x = 0.0F;
        } else if (axis == 1) {
            pos.x -= pen;
            v.x = 0.0F;
        } else if (axis == 2) {
            pos.y += pen;
            v.y = 0.0F;
            rb.SetGrounded(true);
        } else {
            pos.y -= pen;
            v.y = 0.0F;
        }
    }
    tr.SetTranslation(pos);
    rb.SetVelocity(v);
    return true;
}

[[nodiscard]] bool TryResolveCircleVsStaticCircle(
        const float cx,
        const float cy,
        const float cr,
        TransformComponent& tr,
        Rigidbody2DComponent& rb,
        const float scx,
        const float scy,
        const float sr,
        const float restitution) noexcept {
    float dx = cx - scx;
    float dy = cy - scy;
    float d2 = dx * dx + dy * dy;
    const float sum = cr + sr;
    if (d2 > sum * sum + 1.0e-8F) {
        return false;
    }
    Vector3 pos = tr.GetLocalTransform().translation;
    Vector2 v = rb.GetVelocity();
    constexpr float kEps = 1.0e-6F;
    if (d2 < kEps * kEps) {
        pos.x += sum * 0.5F;
        ResolveNormalVelocity(v, 1.0F, 0.0F, restitution);
    } else {
        const float d = std::sqrt(d2);
        const float pen = sum - d;
        const float inv = 1.0F / d;
        const float nx = dx * inv;
        const float ny = dy * inv;
        pos.x += nx * pen;
        pos.y += ny * pen;
        ResolveNormalVelocity(v, nx, ny, restitution);
        if (ny > 0.55F) {
            rb.SetGrounded(true);
        }
    }
    tr.SetTranslation(pos);
    rb.SetVelocity(v);
    return true;
}

[[nodiscard]] bool TryResolveBoxVsStaticPolygon(
        CollisionAabb2& box,
        TransformComponent& tr,
        Rigidbody2DComponent& rb,
        const StaticCollider2D& poly) noexcept {
    float nx = 0.0F;
    float ny = 1.0F;
    float pen = 0.0F;
    if (!TryComputeBoxPolygonSeparation(box, poly, nx, ny, pen)) {
        return false;
    }
    Vector3 pos = tr.GetLocalTransform().translation;
    Vector2 v = rb.GetVelocity();
    pos.x += nx * pen;
    pos.y += ny * pen;
    const float e = RestitutionForStaticContact(poly);
    ResolveNormalVelocity(v, nx, ny, e);
    if (ny > 0.55F) {
        rb.SetGrounded(true);
    }
    tr.SetTranslation(pos);
    rb.SetVelocity(v);
    return true;
}

[[nodiscard]] bool TryResolveBoxWithStaticCollider(
        CollisionAabb2& box,
        TransformComponent& tr,
        Rigidbody2DComponent& rb,
        const Collider2D& col) noexcept {
    const float e = RestitutionForStaticContact(col);
    const ShapeType2D type = col.GetShapeType();
    if (type == ShapeType2D::Box) {
        return TryResolveBoxVsStaticAabb(box, tr, rb, col.GetBounds(), e);
    }
    if (type == ShapeType2D::Circle) {
        const StaticCollider2D snap = col.ToLegacySnapshot();
        return TryResolveBoxVsStaticCircle(box, tr, rb, snap.circleCx, snap.circleCy, snap.circleR, e);
    }
    return TryResolveBoxVsStaticPolygon(box, tr, rb, col.ToLegacySnapshot());
}

[[nodiscard]] bool TryResolveCircleVsStaticPolygon(
        const float cx,
        const float cy,
        const float cr,
        TransformComponent& tr,
        Rigidbody2DComponent& rb,
        const StaticCollider2D& poly) noexcept {
    float nx = 0.0F;
    float ny = 1.0F;
    float pen = 0.0F;
    if (!TryComputeCirclePolygonSeparation(cx, cy, cr, poly, nx, ny, pen)) {
        return false;
    }
    Vector3 pos = tr.GetLocalTransform().translation;
    Vector2 v = rb.GetVelocity();
    pos.x += nx * pen;
    pos.y += ny * pen;
    const float e = RestitutionForStaticContact(poly);
    ResolveNormalVelocity(v, nx, ny, e);
    if (ny > 0.55F) {
        rb.SetGrounded(true);
    }
    tr.SetTranslation(pos);
    rb.SetVelocity(v);
    return true;
}

[[nodiscard]] bool TryResolveCircleWithStaticCollider(
        GameObject& dyn,
        const CircleCollider2DComponent& col,
        TransformComponent& tr,
        Rigidbody2DComponent& rb,
        const Collider2D& st) noexcept {
    float cx = 0.0F;
    float cy = 0.0F;
    float cr = 0.0F;
    ComputeCircleCollider2World(dyn, col, cx, cy, cr);
    const float e = RestitutionForStaticContact(st);
    const ShapeType2D type = st.GetShapeType();
    if (type == ShapeType2D::Box) {
        return TryResolveCircleVsStaticAabb(cx, cy, cr, tr, rb, st.GetBounds(), e);
    }
    if (type == ShapeType2D::Circle) {
        const StaticCollider2D snap = st.ToLegacySnapshot();
        return TryResolveCircleVsStaticCircle(cx, cy, cr, tr, rb, snap.circleCx, snap.circleCy, snap.circleR, e);
    }
    return TryResolveCircleVsStaticPolygon(cx, cy, cr, tr, rb, st.ToLegacySnapshot());
}

void ResolveDynamicBoxVsStatics(
        GameObject& dyn,
        Rigidbody2DComponent& rb,
        TransformComponent& tr,
        BoxCollider2DComponent& col,
        const Array<Collider2D>& colliders,
        SpatialHashGrid2D& broadPhase) {
    rb.SetGrounded(false);
    for (int iter = 0; iter < 6; ++iter) {
        bool any = false;
        CollisionAabb2 box{};
        ComputeBoxCollider2WorldAabb(dyn, col, box);

        Array<std::uint32_t> candidates;
        broadPhase.QueryUniquePayloadIndices(box, candidates);

        for (std::size_t ci = 0; ci < candidates.GetSize(); ++ci) {
            const std::uint32_t si = candidates[ci];
            if (si >= colliders.GetSize()) {
                continue;
            }
            if (!CollisionFilter2D::ShouldCollide(
                        col.GetCategoryBits(),
                        col.GetMaskBits(),
                        colliders[si].GetCategoryBits(),
                        colliders[si].GetMaskBits())) {
                continue;
            }
            if (colliders[si].IsTrigger() || col.GetIsTrigger()) {
                continue;
            }
            if (TryResolveBoxWithStaticCollider(box, tr, rb, colliders[si])) {
                ComputeBoxCollider2WorldAabb(dyn, col, box);
                any = true;
            }
        }
        if (!any) {
            break;
        }
    }

    CollisionAabb2 box{};
    ComputeBoxCollider2WorldAabb(dyn, col, box);
    Array<std::uint32_t> triggerCandidates;
    broadPhase.QueryUniquePayloadIndices(box, triggerCandidates);
    for (std::size_t ci = 0; ci < triggerCandidates.GetSize(); ++ci) {
        const std::uint32_t si = triggerCandidates[ci];
        if (si >= colliders.GetSize()) {
            continue;
        }
        if (!CollisionFilter2D::ShouldCollide(
                    col.GetCategoryBits(),
                    col.GetMaskBits(),
                    colliders[si].GetCategoryBits(),
                    colliders[si].GetMaskBits())) {
            continue;
        }
        if (!colliders[si].IsTrigger() && !col.GetIsTrigger()) {
            continue;
        }
        if (!OverlapDynamicBoxWithStatic(dyn, col, colliders[si])) {
            continue;
        }
        TriggerDispatcher2D::ReportStaticDynamic(dyn, colliders[si], si, col.GetIsTrigger());
    }
}

void ResolveDynamicCircleVsStatics(
        GameObject& dyn,
        Rigidbody2DComponent& rb,
        TransformComponent& tr,
        CircleCollider2DComponent& col,
        const Array<Collider2D>& colliders,
        SpatialHashGrid2D& broadPhase) {
    rb.SetGrounded(false);
    for (int iter = 0; iter < 6; ++iter) {
        bool any = false;
        float cx = 0.0F;
        float cy = 0.0F;
        float cr = 0.0F;
        ComputeCircleCollider2World(dyn, col, cx, cy, cr);
        CollisionAabb2 query{};
        query.minX = cx - cr;
        query.maxX = cx + cr;
        query.minY = cy - cr;
        query.maxY = cy + cr;

        Array<std::uint32_t> candidates;
        broadPhase.QueryUniquePayloadIndices(query, candidates);

        for (std::size_t ci = 0; ci < candidates.GetSize(); ++ci) {
            const std::uint32_t si = candidates[ci];
            if (si >= colliders.GetSize()) {
                continue;
            }
            if (!CollisionFilter2D::ShouldCollide(
                        col.GetCategoryBits(),
                        col.GetMaskBits(),
                        colliders[si].GetCategoryBits(),
                        colliders[si].GetMaskBits())) {
                continue;
            }
            if (colliders[si].IsTrigger() || col.GetIsTrigger()) {
                continue;
            }
            if (TryResolveCircleWithStaticCollider(dyn, col, tr, rb, colliders[si])) {
                any = true;
            }
        }
        if (!any) {
            break;
        }
    }

    float tcx = 0.0F;
    float tcy = 0.0F;
    float tcr = 0.0F;
    ComputeCircleCollider2World(dyn, col, tcx, tcy, tcr);
    CollisionAabb2 triggerQuery{};
    triggerQuery.minX = tcx - tcr;
    triggerQuery.maxX = tcx + tcr;
    triggerQuery.minY = tcy - tcr;
    triggerQuery.maxY = tcy + tcr;
    Array<std::uint32_t> triggerCandidates;
    broadPhase.QueryUniquePayloadIndices(triggerQuery, triggerCandidates);
    for (std::size_t ci = 0; ci < triggerCandidates.GetSize(); ++ci) {
        const std::uint32_t si = triggerCandidates[ci];
        if (si >= colliders.GetSize()) {
            continue;
        }
        if (!CollisionFilter2D::ShouldCollide(
                    col.GetCategoryBits(),
                    col.GetMaskBits(),
                    colliders[si].GetCategoryBits(),
                    colliders[si].GetMaskBits())) {
            continue;
        }
        if (!colliders[si].IsTrigger() && !col.GetIsTrigger()) {
            continue;
        }
        if (!OverlapDynamicCircleWithStatic(dyn, col, colliders[si])) {
            continue;
        }
        TriggerDispatcher2D::ReportStaticDynamic(dyn, colliders[si], si, col.GetIsTrigger());
    }
}

void ComputeDynamicBodyWorldQueryAabb(DynamicBody2D& body, CollisionAabb2& out) noexcept {
    RefreshDynamicBody2D(body);
    out = body.collider.GetBounds();
}

[[nodiscard]] bool GetPrimaryCollider2DFilters(
        const CircleCollider2DComponent* circ,
        const BoxCollider2DComponent* box,
        bool& outTrigger,
        std::uint16_t& outCategory,
        std::uint16_t& outMask) noexcept {
    if (circ != nullptr) {
        outTrigger = circ->GetIsTrigger();
        outCategory = circ->GetCategoryBits();
        outMask = circ->GetMaskBits();
        return true;
    }
    if (box != nullptr) {
        outTrigger = box->GetIsTrigger();
        outCategory = box->GetCategoryBits();
        outMask = box->GetMaskBits();
        return true;
    }
    return false;
}

[[nodiscard]] bool OverlapDynamicPairNarrow(
        GameObject& a,
        BoxCollider2DComponent* boxA,
        CircleCollider2DComponent* circA,
        GameObject& b,
        BoxCollider2DComponent* boxB,
        CircleCollider2DComponent* circB) noexcept {
    if (circA != nullptr) {
        float ax = 0.0F;
        float ay = 0.0F;
        float ar = 0.0F;
        ComputeCircleCollider2World(a, *circA, ax, ay, ar);
        if (circB != nullptr) {
            float bx = 0.0F;
            float by = 0.0F;
            float br = 0.0F;
            ComputeCircleCollider2World(b, *circB, bx, by, br);
            return CollisionCirclesOverlap(ax, ay, ar, bx, by, br);
        }
        if (boxB != nullptr) {
            CollisionAabb2 bb{};
            ComputeBoxCollider2WorldAabb(b, *boxB, bb);
            return CollisionAabb2OverlapsCircle(bb, ax, ay, ar);
        }
        return false;
    }
    if (boxA != nullptr) {
        CollisionAabb2 ba{};
        ComputeBoxCollider2WorldAabb(a, *boxA, ba);
        if (circB != nullptr) {
            float bx = 0.0F;
            float by = 0.0F;
            float br = 0.0F;
            ComputeCircleCollider2World(b, *circB, bx, by, br);
            return CollisionAabb2OverlapsCircle(ba, bx, by, br);
        }
        if (boxB != nullptr) {
            CollisionAabb2 bb{};
            ComputeBoxCollider2WorldAabb(b, *boxB, bb);
            return CollisionAabb2Overlaps(ba, bb);
        }
    }
    return false;
}

void TrySeparateBoxBox2D(
        const CollisionAabb2& a,
        const CollisionAabb2& b,
        TransformComponent& trA,
        TransformComponent& trB) noexcept {
    const float overlapX = (std::min)(a.maxX, b.maxX) - (std::max)(a.minX, b.minX);
    const float overlapY = (std::min)(a.maxY, b.maxY) - (std::max)(a.minY, b.minY);
    if (overlapX <= 0.0F || overlapY <= 0.0F) {
        return;
    }
    const float ca = 0.5F * (a.minX + a.maxX);
    const float cb = 0.5F * (b.minX + b.maxX);
    const float cya = 0.5F * (a.minY + a.maxY);
    const float cyb = 0.5F * (b.minY + b.maxY);
    Vector3 pa = trA.GetLocalTransform().translation;
    Vector3 pb = trB.GetLocalTransform().translation;
    if (overlapX < overlapY) {
        const float dir = (ca < cb) ? -1.0F : 1.0F;
        const float mx = dir * overlapX * 0.5F;
        pa.x += mx;
        pb.x -= mx;
    } else {
        const float dir = (cya < cyb) ? -1.0F : 1.0F;
        const float my = dir * overlapY * 0.5F;
        pa.y += my;
        pb.y -= my;
    }
    trA.SetTranslation(pa);
    trB.SetTranslation(pb);
}

void TrySeparateCircleCircle2D(
        TransformComponent& trA,
        const float ax,
        const float ay,
        const float ar,
        TransformComponent& trB,
        const float bx,
        const float by,
        const float br) noexcept {
    float dx = bx - ax;
    float dy = by - ay;
    float d2 = dx * dx + dy * dy;
    constexpr float kEps = 1.0e-8F;
    Vector3 pa = trA.GetLocalTransform().translation;
    Vector3 pb = trB.GetLocalTransform().translation;
    const float sum = ar + br;
    if (d2 < kEps * kEps) {
        const float push = sum * 0.25F;
        pa.x -= push;
        pb.x += push;
        trA.SetTranslation(pa);
        trB.SetTranslation(pb);
        return;
    }
    const float d = std::sqrt(d2);
    const float pen = sum - d;
    if (pen <= 0.0F) {
        return;
    }
    dx /= d;
    dy /= d;
    pa.x -= dx * pen * 0.5F;
    pa.y -= dy * pen * 0.5F;
    pb.x += dx * pen * 0.5F;
    pb.y += dy * pen * 0.5F;
    trA.SetTranslation(pa);
    trB.SetTranslation(pb);
}

void TrySeparateBoxCircleWorld2D(
        const CollisionAabb2& box,
        TransformComponent& trBox,
        const float cx,
        const float cy,
        const float cr,
        TransformComponent& trCircle) noexcept {
    const float qx = std::clamp(cx, box.minX, box.maxX);
    const float qy = std::clamp(cy, box.minY, box.maxY);
    float dx = cx - qx;
    float dy = cy - qy;
    float d2 = dx * dx + dy * dy;
    constexpr float kEps = 1.0e-6F;
    Vector3 pb = trBox.GetLocalTransform().translation;
    Vector3 pc = trCircle.GetLocalTransform().translation;
    if (d2 < kEps * kEps) {
        const float penD = cy - box.minY;
        const float penU = box.maxY - cy;
        const float penL = cx - box.minX;
        const float penR = box.maxX - cx;
        float m = penD;
        int axis = 0;
        if (penU < m) {
            m = penU;
            axis = 1;
        }
        if (penL < m) {
            m = penL;
            axis = 2;
        }
        if (penR < m) {
            m = penR;
            axis = 3;
        }
        const float half = (std::max)(m, 1.0e-4F) * 0.5F;
        if (axis == 0) {
            pc.y -= half;
            pb.y += half;
        } else if (axis == 1) {
            pc.y += half;
            pb.y -= half;
        } else if (axis == 2) {
            pc.x -= half;
            pb.x += half;
        } else {
            pc.x += half;
            pb.x -= half;
        }
        trBox.SetTranslation(pb);
        trCircle.SetTranslation(pc);
        return;
    }
    const float d = std::sqrt(d2);
    const float pen = cr - d;
    if (pen <= 0.0F) {
        return;
    }
    dx /= d;
    dy /= d;
    pc.x += dx * pen * 0.5F;
    pc.y += dy * pen * 0.5F;
    pb.x -= dx * pen * 0.5F;
    pb.y -= dy * pen * 0.5F;
    trBox.SetTranslation(pb);
    trCircle.SetTranslation(pc);
}

void TrySeparateDynamicPair2D(
        GameObject& a,
        TransformComponent& trA,
        BoxCollider2DComponent* boxA,
        CircleCollider2DComponent* circA,
        GameObject& b,
        TransformComponent& trB,
        BoxCollider2DComponent* boxB,
        CircleCollider2DComponent* circB) noexcept {
    if (circA != nullptr && circB != nullptr) {
        float ax = 0.0F;
        float ay = 0.0F;
        float ar = 0.0F;
        float bx = 0.0F;
        float by = 0.0F;
        float br = 0.0F;
        ComputeCircleCollider2World(a, *circA, ax, ay, ar);
        ComputeCircleCollider2World(b, *circB, bx, by, br);
        TrySeparateCircleCircle2D(trA, ax, ay, ar, trB, bx, by, br);
        return;
    }
    if (circA != nullptr && boxB != nullptr) {
        CollisionAabb2 bb{};
        ComputeBoxCollider2WorldAabb(b, *boxB, bb);
        float ax = 0.0F;
        float ay = 0.0F;
        float ar = 0.0F;
        ComputeCircleCollider2World(a, *circA, ax, ay, ar);
        TrySeparateBoxCircleWorld2D(bb, trB, ax, ay, ar, trA);
        return;
    }
    if (boxA != nullptr && circB != nullptr) {
        CollisionAabb2 ba{};
        ComputeBoxCollider2WorldAabb(a, *boxA, ba);
        float bx = 0.0F;
        float by = 0.0F;
        float br = 0.0F;
        ComputeCircleCollider2World(b, *circB, bx, by, br);
        TrySeparateBoxCircleWorld2D(ba, trA, bx, by, br, trB);
        return;
    }
    if (boxA != nullptr && boxB != nullptr) {
        CollisionAabb2 ba{};
        CollisionAabb2 bb{};
        ComputeBoxCollider2WorldAabb(a, *boxA, ba);
        ComputeBoxCollider2WorldAabb(b, *boxB, bb);
        TrySeparateBoxBox2D(ba, bb, trA, trB);
    }
}

void ProcessOneDynamicDynamicPair2D(
        DynamicBody2D& bi,
        DynamicBody2D& bj,
        const PhysicsWorld2DSettings& settings) noexcept {
    RefreshDynamicBody2D(bi);
    RefreshDynamicBody2D(bj);
    GameObject& a = *bi.object;
    GameObject& b = *bj.object;
    CircleCollider2DComponent* ca = bi.circle;
    BoxCollider2DComponent* ba = bi.box;
    CircleCollider2DComponent* cb = bj.circle;
    BoxCollider2DComponent* bb = bj.box;

    const bool trigA = bi.collider.GetFilter().isTrigger;
    const bool trigB = bj.collider.GetFilter().isTrigger;
    const std::uint16_t catA = bi.collider.GetFilter().categoryBits;
    const std::uint16_t maskA = bi.collider.GetFilter().maskBits;
    const std::uint16_t catB = bj.collider.GetFilter().categoryBits;
    const std::uint16_t maskB = bj.collider.GetFilter().maskBits;
    if (!CollisionFilter2D::ShouldCollide(catA, maskA, catB, maskB)) {
        return;
    }
    if (!OverlapDynamicPairNarrow(a, ba, ca, b, bb, cb)) {
        return;
    }

    if (trigA || trigB) {
        TriggerDispatcher2D::ReportDynamicDynamic(a, b, trigA, trigB);
    }
    if (!trigA && !trigB && settings.resolveDynamicVsDynamic) {
        TrySeparateDynamicPair2D(a, *bi.transform, ba, ca, b, *bj.transform, bb, cb);
    }
}

void ProcessDynamicDynamicPairs2D(
        Array<DynamicBody2D>& bodies,
        const PhysicsWorld2DSettings& settings,
        const float broadPhaseCellSize,
        SpatialHashGrid2D& dynBroad,
        Array<std::uint32_t>& pairCandidatesScratch) noexcept {
    const std::size_t n = bodies.GetSize();
    if (n < 2) {
        return;
    }

    dynBroad.Clear();
    dynBroad.SetCellSize(broadPhaseCellSize);
    for (std::size_t i = 0; i < n; ++i) {
        CollisionAabb2 aabb{};
        ComputeDynamicBodyWorldQueryAabb(bodies[i], aabb);
        dynBroad.InsertIndexedAabb(static_cast<std::uint32_t>(i), aabb);
    }

    for (std::size_t i = 0; i < n; ++i) {
        CollisionAabb2 query{};
        ComputeDynamicBodyWorldQueryAabb(bodies[i], query);
        dynBroad.QueryUniquePayloadIndices(query, pairCandidatesScratch);
        for (std::size_t k = 0; k < pairCandidatesScratch.GetSize(); ++k) {
            const std::uint32_t j32 = pairCandidatesScratch[k];
            if (j32 <= static_cast<std::uint32_t>(i)) {
                continue;
            }
            const std::size_t j = static_cast<std::size_t>(j32);
            if (j >= n) {
                continue;
            }
            ProcessOneDynamicDynamicPair2D(bodies[i], bodies[j], settings);
        }
    }
}

}  // namespace ContactResolver2DDetail

void ContactResolver2D::ResolveAllDynamicsAgainstStatics(
        GameWorld& world,
        const Array<Collider2D>& colliders,
        SpatialHashGrid2D& broadPhase) {
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        Rigidbody2DComponent* rb = o->GetComponent<Rigidbody2DComponent>();
        TransformComponent* tr = o->GetComponent<TransformComponent>();
        if (rb == nullptr || tr == nullptr) {
            return;
        }
        if (rb->GetBodyType() != RigidbodyBodyType2D::Dynamic) {
            return;
        }
        CircleCollider2DComponent* circleCol = o->GetComponent<CircleCollider2DComponent>();
        BoxCollider2DComponent* boxCol = o->GetComponent<BoxCollider2DComponent>();
        if (circleCol != nullptr) {
            ContactResolver2DDetail::ResolveDynamicCircleVsStatics(
                    *o, *rb, *tr, *circleCol, colliders, broadPhase);
            return;
        }
        if (boxCol != nullptr) {
            ContactResolver2DDetail::ResolveDynamicBoxVsStatics(
                    *o, *rb, *tr, *boxCol, colliders, broadPhase);
        }
    });
}

void ContactResolver2D::ResolveDynamicDynamicPairs(
        Array<DynamicBody2D>& bodies,
        const PhysicsWorld2DSettings& settings,
        const float broadPhaseCellSize,
        SpatialHashGrid2D& dynBroad,
        Array<std::uint32_t>& pairCandidatesScratch) noexcept {
    ContactResolver2DDetail::ProcessDynamicDynamicPairs2D(
            bodies, settings, broadPhaseCellSize, dynBroad, pairCandidatesScratch);
}

}  // namespace Spark
