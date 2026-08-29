#include "spark/scene/editor/TransformGizmo.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Transform.hpp"
#include "spark/scene/mesh/MeshRaycast.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 GizmoAxisDir(const int axis) noexcept {
    if (axis == 0) {
        return Vector3::UnitX;
    }
    if (axis == 1) {
        return Vector3::UnitY;
    }
    return Vector3::UnitZ;
}

[[nodiscard]] float GizmoAxisLength(const float objectExtent) noexcept {
    return std::max(1.15F, objectExtent * 1.2F);
}

[[nodiscard]] bool ClosestRayLineParameter(
        const Vector3& rayOrigin,
        const Vector3& rayDir,
        const Vector3& pivot,
        const Vector3& axis,
        float& outLineS) noexcept {
    const Vector3 w{rayOrigin.x - pivot.x, rayOrigin.y - pivot.y, rayOrigin.z - pivot.z};
    const float a = rayDir.x * rayDir.x + rayDir.y * rayDir.y + rayDir.z * rayDir.z;
    const float b = rayDir.x * axis.x + rayDir.y * axis.y + rayDir.z * axis.z;
    const float c = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
    const float d = rayDir.x * w.x + rayDir.y * w.y + rayDir.z * w.z;
    const float e = axis.x * w.x + axis.y * w.y + axis.z * w.z;
    const float denom = a * c - b * b;
    if (std::fabs(denom) < 1.0e-8F) {
        return false;
    }
    outLineS = (b * d - a * e) / denom;
    return true;
}

[[nodiscard]] bool TryPickAxisHandles(
        const Vector3& rayOrigin,
        const Vector3& rayDir,
        const Vector3& pivot,
        const float objectExtent,
        int& outAxis) noexcept {
    const float len = GizmoAxisLength(objectExtent);
    const float pickR = std::max(0.18F, len * 0.16F);
    float bestT = 1.0e30F;
    int bestAxis = -1;
    for (int axis = 0; axis < 3; ++axis) {
        const Vector3 dir = GizmoAxisDir(axis);
        const Vector3 tip{pivot.x + dir.x * len, pivot.y + dir.y * len, pivot.z + dir.z * len};
        float t = 0.0F;
        if (TryRaycastSphereWorld(rayOrigin, rayDir, tip, pickR, 1.0e-4F, bestT, t)) {
            bestT = t;
            bestAxis = axis;
        }
    }
    outAxis = bestAxis;
    return bestAxis >= 0;
}

[[nodiscard]] bool RayIntersectPlane(
        const Vector3& ro,
        const Vector3& rd,
        const Vector3& planePoint,
        const Vector3& planeNormal,
        Vector3& outHit) noexcept {
    const float denom = Vector3::Dot(rd, planeNormal);
    if (std::fabs(denom) < 1.0e-6F) {
        return false;
    }
    const Vector3 delta{planePoint.x - ro.x, planePoint.y - ro.y, planePoint.z - ro.z};
    const float t = Vector3::Dot(delta, planeNormal) / denom;
    if (t < 0.0F || t > 800.0F) {
        return false;
    }
    outHit = {ro.x + rd.x * t, ro.y + rd.y * t, ro.z + rd.z * t};
    return true;
}

[[nodiscard]] float PlaneAngleAboutAxis(
        const Vector3& pivot,
        const Vector3& axis,
        const Vector3& point) noexcept {
    Vector3 radial{
            point.x - pivot.x,
            point.y - pivot.y,
            point.z - pivot.z};
    const float along = Vector3::Dot(radial, axis);
    radial.x -= axis.x * along;
    radial.y -= axis.y * along;
    radial.z -= axis.z * along;
    if (radial.LengthSquared() < 1.0e-8F) {
        return 0.0F;
    }
    radial = radial.Normalized();
    Vector3 reference = Vector3::Cross(axis, Vector3::UnitY);
    if (reference.LengthSquared() < 1.0e-6F) {
        reference = Vector3::Cross(axis, Vector3::UnitX);
    }
    reference = reference.Normalized();
    const Vector3 tangent = Vector3::Cross(axis, reference).Normalized();
    const float x = Vector3::Dot(radial, reference);
    const float y = Vector3::Dot(radial, tangent);
    return std::atan2(y, x);
}

[[nodiscard]] bool TryPickRotateRing(
        const Vector3& rayOrigin,
        const Vector3& rayDir,
        const Vector3& pivot,
        const float objectExtent,
        int& outAxis) noexcept {
    const float ringRadius = GizmoAxisLength(objectExtent) * 0.82F;
    const float pickThickness = std::max(0.14F, ringRadius * 0.14F);
    float bestDistance = 1.0e30F;
    int bestAxis = -1;
    for (int axis = 0; axis < 3; ++axis) {
        const Vector3 normal = GizmoAxisDir(axis);
        Vector3 hit{};
        if (!RayIntersectPlane(rayOrigin, rayDir, pivot, normal, hit)) {
            continue;
        }
        const Vector3 delta{hit.x - pivot.x, hit.y - pivot.y, hit.z - pivot.z};
        const float radial = std::sqrt(std::max(0.0F, delta.LengthSquared() - Vector3::Dot(delta, normal) * Vector3::Dot(delta, normal)));
        const float ringError = std::fabs(radial - ringRadius);
        if (ringError <= pickThickness && ringError < bestDistance) {
            bestDistance = ringError;
            bestAxis = axis;
        }
    }
    outAxis = bestAxis;
    return bestAxis >= 0;
}

void AppendAxisShaftAndKnob(
        const Vector3& pivot,
        const float len,
        const Vector3& dir,
        const Vector3& color,
        const bool hot,
        Array<SceneDrawItem>& out) {
    const float shaft = std::max(0.05F, len * 0.055F);
    const float emissiveBoost = hot ? 4.5F : 2.8F;

    SceneDrawItem shaftDraw{};
    shaftDraw.mesh = SceneMeshSlot::UnitCube;
    Vector3 shaftScale{shaft, shaft, shaft};
    if (dir.x != 0.0F) {
        shaftScale.x = len;
    } else if (dir.y != 0.0F) {
        shaftScale.y = len;
    } else {
        shaftScale.z = len;
    }
    const Vector3 shaftCenter{
            pivot.x + dir.x * len * 0.5F,
            pivot.y + dir.y * len * 0.5F,
            pivot.z + dir.z * len * 0.5F};
    shaftDraw.model = Matrix4::Translation(shaftCenter) * Matrix4::Scale(shaftScale);
    shaftDraw.albedo = color;
    shaftDraw.metallic = 0.0F;
    shaftDraw.roughness = 0.35F;
    shaftDraw.emissiveColor = color;
    shaftDraw.emissiveIntensity = emissiveBoost;
    out.PushBack(shaftDraw);

    SceneDrawItem knob{};
    knob.mesh = SceneMeshSlot::UnitCube;
    const float knobS = shaft * 2.8F;
    const Vector3 tip{pivot.x + dir.x * len, pivot.y + dir.y * len, pivot.z + dir.z * len};
    knob.model = Matrix4::Translation(tip) * Matrix4::Scale(knobS);
    knob.albedo = color;
    knob.metallic = 0.0F;
    knob.roughness = 0.25F;
    knob.emissiveColor = {
            std::min(1.0F, color.x + 0.2F),
            std::min(1.0F, color.y + 0.2F),
            std::min(1.0F, color.z + 0.2F)};
    knob.emissiveIntensity = emissiveBoost + (hot ? 2.0F : 0.0F);
    out.PushBack(knob);
}

void AppendTranslateDraws(
        const Vector3& pivot,
        const float objectExtent,
        const int highlightAxis,
        Array<SceneDrawItem>& out) {
    const float len = GizmoAxisLength(objectExtent);
    static constexpr Vector3 kColors[3] = {
            {0.95F, 0.22F, 0.20F},
            {0.28F, 0.92F, 0.34F},
            {0.30F, 0.52F, 0.98F},
    };
    for (int axis = 0; axis < 3; ++axis) {
        AppendAxisShaftAndKnob(pivot, len, GizmoAxisDir(axis), kColors[axis], axis == highlightAxis, out);
    }
}

void AppendScaleDraws(
        const Vector3& pivot,
        const float objectExtent,
        const int highlightAxis,
        Array<SceneDrawItem>& out) {
    const float len = GizmoAxisLength(objectExtent);
    static constexpr Vector3 kColors[3] = {
            {0.95F, 0.35F, 0.30F},
            {0.35F, 0.95F, 0.40F},
            {0.40F, 0.60F, 0.98F},
    };
    for (int axis = 0; axis < 3; ++axis) {
        const Vector3 dir = GizmoAxisDir(axis);
        const bool hot = axis == highlightAxis;
        const float box = std::max(0.08F, len * 0.09F);
        SceneDrawItem cube{};
        cube.mesh = SceneMeshSlot::UnitCube;
        const Vector3 tip{pivot.x + dir.x * len, pivot.y + dir.y * len, pivot.z + dir.z * len};
        cube.model = Matrix4::Translation(tip) * Matrix4::Scale(box);
        cube.albedo = kColors[axis];
        cube.emissiveColor = kColors[axis];
        cube.emissiveIntensity = hot ? 5.0F : 3.0F;
        out.PushBack(cube);

        SceneDrawItem line{};
        line.mesh = SceneMeshSlot::UnitCube;
        Vector3 scale{box * 0.35F, box * 0.35F, box * 0.35F};
        if (dir.x != 0.0F) {
            scale.x = len;
        } else if (dir.y != 0.0F) {
            scale.y = len;
        } else {
            scale.z = len;
        }
        const Vector3 center{
                pivot.x + dir.x * len * 0.5F,
                pivot.y + dir.y * len * 0.5F,
                pivot.z + dir.z * len * 0.5F};
        line.model = Matrix4::Translation(center) * Matrix4::Scale(scale);
        line.albedo = kColors[axis];
        line.emissiveColor = kColors[axis];
        line.emissiveIntensity = hot ? 3.5F : 2.0F;
        out.PushBack(line);
    }
}

void AppendRotateDraws(
        const Vector3& pivot,
        const float objectExtent,
        const int highlightAxis,
        Array<SceneDrawItem>& out) {
    const float ringRadius = GizmoAxisLength(objectExtent) * 0.82F;
    static constexpr Vector3 kColors[3] = {
            {0.95F, 0.22F, 0.20F},
            {0.28F, 0.92F, 0.34F},
            {0.30F, 0.52F, 0.98F},
    };
    constexpr int kSegments = 24;
    for (int axis = 0; axis < 3; ++axis) {
        const Vector3 normal = GizmoAxisDir(axis);
        Vector3 reference = Vector3::Cross(normal, Vector3::UnitY);
        if (reference.LengthSquared() < 1.0e-6F) {
            reference = Vector3::Cross(normal, Vector3::UnitX);
        }
        reference = reference.Normalized();
        const Vector3 tangent = Vector3::Cross(normal, reference).Normalized();
        const Vector3 color = kColors[axis];
        const bool hot = axis == highlightAxis;
        const float bead = std::max(0.04F, ringRadius * 0.045F);
        for (int seg = 0; seg < kSegments; ++seg) {
            const float angle = static_cast<float>(seg) / static_cast<float>(kSegments) * 6.2831853F;
            const float c = std::cos(angle);
            const float s = std::sin(angle);
            const Vector3 point{
                    pivot.x + (reference.x * c + tangent.x * s) * ringRadius,
                    pivot.y + (reference.y * c + tangent.y * s) * ringRadius,
                    pivot.z + (reference.z * c + tangent.z * s) * ringRadius};
            SceneDrawItem beadDraw{};
            beadDraw.mesh = SceneMeshSlot::UnitCube;
            beadDraw.model = Matrix4::Translation(point) * Matrix4::Scale(bead);
            beadDraw.albedo = color;
            beadDraw.emissiveColor = color;
            beadDraw.emissiveIntensity = hot ? 4.0F : 2.5F;
            out.PushBack(beadDraw);
        }
    }
}

}  // namespace

void TransformGizmo::CycleMode() noexcept {
    switch (mode) {
        case TransformGizmoMode::Translate:
            mode = TransformGizmoMode::Rotate;
            break;
        case TransformGizmoMode::Rotate:
            mode = TransformGizmoMode::Scale;
            break;
        case TransformGizmoMode::Scale:
        default:
            mode = TransformGizmoMode::Translate;
            break;
    }
}

bool TransformGizmo::TryBeginDrag(
        const Vector3& rayOrigin,
        const Vector3& rayDir,
        const TransformComponent& transform,
        const float objectExtent,
        GameObject& target) noexcept {
    const Transform& local = transform.GetLocalTransform();
    const Vector3 pivot = local.translation;
    int axis = -1;
    bool picked = false;
    switch (mode) {
        case TransformGizmoMode::Translate:
        case TransformGizmoMode::Scale:
            picked = TryPickAxisHandles(rayOrigin, rayDir, pivot, objectExtent, axis);
            break;
        case TransformGizmoMode::Rotate:
            picked = TryPickRotateRing(rayOrigin, rayDir, pivot, objectExtent, axis);
            break;
    }
    if (!picked) {
        return false;
    }

    drag.active = true;
    drag.activeAxis = axis;
    drag.target = &target;
    drag.pivot = pivot;
    drag.startTranslation = local.translation;
    drag.startRotation = local.rotation;
    drag.startScale = local.scale;
    drag.startLineS = 0.0F;
    drag.startPlaneAngle = 0.0F;

    if (mode == TransformGizmoMode::Translate || mode == TransformGizmoMode::Scale) {
        (void)ClosestRayLineParameter(rayOrigin, rayDir, pivot, GizmoAxisDir(axis), drag.startLineS);
    } else {
        Vector3 hit{};
        if (RayIntersectPlane(rayOrigin, rayDir, pivot, GizmoAxisDir(axis), hit)) {
            drag.startPlaneAngle = PlaneAngleAboutAxis(pivot, GizmoAxisDir(axis), hit);
        }
    }
    return true;
}

bool TransformGizmo::UpdateDrag(
        const Vector3& rayOrigin,
        const Vector3& rayDir,
        TransformComponent& transform) noexcept {
    if (!drag.active || drag.activeAxis < 0) {
        return false;
    }

    const Vector3 axis = GizmoAxisDir(drag.activeAxis);
    switch (mode) {
        case TransformGizmoMode::Translate: {
            float lineS = 0.0F;
            if (!ClosestRayLineParameter(rayOrigin, rayDir, drag.pivot, axis, lineS)) {
                return false;
            }
            const float ds = lineS - drag.startLineS;
            transform.SetTranslation({
                    drag.startTranslation.x + axis.x * ds,
                    drag.startTranslation.y + axis.y * ds,
                    drag.startTranslation.z + axis.z * ds});
            return true;
        }
        case TransformGizmoMode::Scale: {
            float lineS = 0.0F;
            if (!ClosestRayLineParameter(rayOrigin, rayDir, drag.pivot, axis, lineS)) {
                return false;
            }
            const float ds = lineS - drag.startLineS;
            Vector3 scale = drag.startScale;
            const float sensitivity = 0.65F;
            if (drag.activeAxis == 0) {
                scale.x = std::max(0.05F, drag.startScale.x + ds * sensitivity);
            } else if (drag.activeAxis == 1) {
                scale.y = std::max(0.05F, drag.startScale.y + ds * sensitivity);
            } else {
                scale.z = std::max(0.05F, drag.startScale.z + ds * sensitivity);
            }
            transform.SetScale(scale);
            return true;
        }
        case TransformGizmoMode::Rotate: {
            Vector3 hit{};
            if (!RayIntersectPlane(rayOrigin, rayDir, drag.pivot, axis, hit)) {
                return false;
            }
            const float angle = PlaneAngleAboutAxis(drag.pivot, axis, hit);
            const float delta = angle - drag.startPlaneAngle;
            const Quaternion deltaQ = Quaternion::FromAxisAngle(axis, delta);
            transform.SetRotation((deltaQ * drag.startRotation).Normalized());
            return true;
        }
    }
    return false;
}

void TransformGizmo::EndDrag() noexcept {
    drag = DragState{};
}

void TransformGizmo::AppendDraws(
        const TransformComponent& transform,
        const float objectExtent,
        Array<SceneDrawItem>& out) const {
    const Vector3 pivot = transform.GetLocalTransform().translation;
    switch (mode) {
        case TransformGizmoMode::Translate:
            AppendTranslateDraws(pivot, objectExtent, drag.activeAxis, out);
            break;
        case TransformGizmoMode::Rotate:
            AppendRotateDraws(pivot, objectExtent, drag.activeAxis, out);
            break;
        case TransformGizmoMode::Scale:
            AppendScaleDraws(pivot, objectExtent, drag.activeAxis, out);
            break;
    }
}

const char* TransformGizmo::ModeName(const TransformGizmoMode mode) noexcept {
    switch (mode) {
        case TransformGizmoMode::Translate:
            return "Move";
        case TransformGizmoMode::Rotate:
            return "Rotate";
        case TransformGizmoMode::Scale:
            return "Scale";
    }
    return "Move";
}

const char* TransformGizmo::GetInteractionHint() const noexcept {
    switch (mode) {
        case TransformGizmoMode::Translate:
            return "Drag axis arrows to move. W/E/R switch gizmo mode.";
        case TransformGizmoMode::Rotate:
            return "Drag colored rings to rotate. W/E/R switch gizmo mode.";
        case TransformGizmoMode::Scale:
            return "Drag axis cubes to scale. W/E/R switch gizmo mode.";
    }
    return "";
}

}  // namespace Spark
