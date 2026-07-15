#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/MeshRaycast.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {
namespace {

/** Orbit <c>cam</c> around <c>pivot</c> while preserving <c>orbitDistance</c>. */
inline void OrbitFlyCameraAroundPivot(
        Spark::FlyCamera& cam,
        const Spark::Vector3& pivot,
        float& orbitDistance,
        const float deltaX,
        const float deltaY) noexcept {
    cam.AddLook(deltaX, deltaY);
    orbitDistance = std::max(1.5F, orbitDistance);
    const Spark::Vector3 f = cam.Forward();
    cam.position = {
            pivot.x - f.x * orbitDistance,
            pivot.y - f.y * orbitDistance,
            pivot.z - f.z * orbitDistance};
}

/** Screen-space pan: moves position on the camera right/up plane. */
inline void PanFlyCamera(
        Spark::FlyCamera& cam,
        const float deltaX,
        const float deltaY,
        const float panScale) noexcept {
    const Spark::Vector3 forward = cam.Forward();
    Spark::Vector3 right = Spark::Vector3::Cross(forward, Spark::Vector3::UnitY);
    if (right.LengthSquared() < Spark::Epsilon) {
        right = Spark::Vector3::UnitX;
    } else {
        right = right.Normalized();
    }
    const Spark::Vector3 up = Spark::Vector3::Cross(right, forward).Normalized();
    cam.position -= right * (deltaX * panScale) + up * (deltaY * panScale);
}

[[nodiscard]] inline Spark::Vector3 SceneEditorGizmoAxisDir(const int axis) noexcept {
    if (axis == 0) {
        return Spark::Vector3::UnitX;
    }
    if (axis == 1) {
        return Spark::Vector3::UnitY;
    }
    return Spark::Vector3::UnitZ;
}

[[nodiscard]] inline float SceneEditorGizmoAxisLength(const float objectExtent) noexcept {
    return std::max(1.15F, objectExtent * 1.2F);
}

/** Ray vs line <c>pivot + axis * s</c>; returns line parameter <c>s</c> at closest approach. */
[[nodiscard]] inline bool ClosestRayLineParameter(
        const Spark::Vector3& rayOrigin,
        const Spark::Vector3& rayDir,
        const Spark::Vector3& pivot,
        const Spark::Vector3& axis,
        float& outLineS) noexcept {
    const Spark::Vector3 w{rayOrigin.x - pivot.x, rayOrigin.y - pivot.y, rayOrigin.z - pivot.z};
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

/** Handle spheres at +X/+Y/+Z tips; returns axis index 0..2. */
[[nodiscard]] inline bool TryPickTranslateGizmo(
        const Spark::Vector3& rayOrigin,
        const Spark::Vector3& rayDir,
        const Spark::Vector3& pivot,
        const float objectExtent,
        int& outAxis) noexcept {
    const float len = SceneEditorGizmoAxisLength(objectExtent);
    const float pickR = std::max(0.18F, len * 0.16F);
    float bestT = 1.0e30F;
    int bestAxis = -1;
    for (int axis = 0; axis < 3; ++axis) {
        const Spark::Vector3 dir = SceneEditorGizmoAxisDir(axis);
        const Spark::Vector3 tip{pivot.x + dir.x * len, pivot.y + dir.y * len, pivot.z + dir.z * len};
        float t = 0.0F;
        if (TryRaycastSphereWorld(rayOrigin, rayDir, tip, pickR, 1.0e-4F, bestT, t)) {
            bestT = t;
            bestAxis = axis;
        }
    }
    outAxis = bestAxis;
    return bestAxis >= 0;
}

inline void AppendTranslateGizmoDraws(
        const Spark::Vector3& pivot,
        const float objectExtent,
        const int highlightAxis,
        Spark::Array<Spark::SceneDrawItem>& out) {
    const float len = SceneEditorGizmoAxisLength(objectExtent);
    const float shaft = std::max(0.05F, len * 0.055F);
    struct AxisVisual {
        Spark::Vector3 dir;
        Spark::Vector3 color;
    };
    static constexpr AxisVisual kAxes[3] = {
            {Spark::Vector3::UnitX, {0.95F, 0.22F, 0.20F}},
            {Spark::Vector3::UnitY, {0.28F, 0.92F, 0.34F}},
            {Spark::Vector3::UnitZ, {0.30F, 0.52F, 0.98F}},
    };
    for (int axis = 0; axis < 3; ++axis) {
        const Spark::Vector3 dir = kAxes[axis].dir;
        const Spark::Vector3 color = kAxes[axis].color;
        const bool hot = (axis == highlightAxis);
        const float emissiveBoost = hot ? 4.5F : 2.8F;

        Spark::SceneDrawItem shaftDraw{};
        shaftDraw.mesh = Spark::SceneMeshSlot::UnitCube;
        Spark::Vector3 shaftScale{shaft, shaft, shaft};
        if (axis == 0) {
            shaftScale.x = len;
        } else if (axis == 1) {
            shaftScale.y = len;
        } else {
            shaftScale.z = len;
        }
        const Spark::Vector3 shaftCenter{
                pivot.x + dir.x * len * 0.5F,
                pivot.y + dir.y * len * 0.5F,
                pivot.z + dir.z * len * 0.5F};
        shaftDraw.model = Spark::Matrix4::Translation(shaftCenter) * Spark::Matrix4::Scale(shaftScale);
        shaftDraw.albedo = color;
        shaftDraw.metallic = 0.0F;
        shaftDraw.roughness = 0.35F;
        shaftDraw.emissiveColor = color;
        shaftDraw.emissiveIntensity = emissiveBoost;
        out.PushBack(shaftDraw);

        Spark::SceneDrawItem knob{};
        knob.mesh = Spark::SceneMeshSlot::UnitCube;
        const float knobS = shaft * 2.8F;
        const Spark::Vector3 tip{pivot.x + dir.x * len, pivot.y + dir.y * len, pivot.z + dir.z * len};
        knob.model = Spark::Matrix4::Translation(tip) * Spark::Matrix4::Scale(knobS);
        knob.albedo = color;
        knob.metallic = 0.0F;
        knob.roughness = 0.25F;
        knob.emissiveColor = {std::min(1.0F, color.x + 0.2F), std::min(1.0F, color.y + 0.2F),
                std::min(1.0F, color.z + 0.2F)};
        knob.emissiveIntensity = emissiveBoost + (hot ? 2.0F : 0.0F);
        out.PushBack(knob);
    }
}

[[nodiscard]] bool RayIntersectPlaneY(
        const Spark::Vector3& ro, const Spark::Vector3& rd, const float planeY, Spark::Vector3& outHit) noexcept {
    if (std::fabs(rd.y) < 1.0e-6F) {
        return false;
    }
    const float t = (planeY - ro.y) / rd.y;
    if (t < 0.0F || t > 800.0F) {
        return false;
    }
    outHit = {ro.x + rd.x * t, ro.y + rd.y * t, ro.z + rd.z * t};
    return true;
}

}  // namespace
}  // namespace Spark
