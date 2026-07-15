#include "spark/scripting/SparkInterop.h"
#include "spark/scripting/SparkInteropInternal.hpp"

#include "spark/math/Matrix4.hpp"
#include "spark/scene/Camera2D.hpp"

#include <cmath>

#include <cstring>

using namespace Spark::Scripting;

extern "C" {

void spark_mat4_perspective_vulkan(
        SparkMatrix4* out,
        const float verticalFovYRad,
        const float aspect,
        const float nearZ,
        const float farZ) {
    if (out == nullptr) {
        return;
    }
    *out = FromMatrix4(Spark::Matrix4::PerspectiveVulkan(verticalFovYRad, aspect, nearZ, farZ));
}

void spark_mat4_orthographic_vulkan(
        SparkMatrix4* out,
        const float left,
        const float right,
        const float bottom,
        const float top,
        const float nearZ,
        const float farZ) {
    if (out == nullptr) {
        return;
    }
    *out = FromMatrix4(Spark::Matrix4::OrthographicVulkan(left, right, bottom, top, nearZ, farZ));
}

void spark_mat4_mul(SparkMatrix4* out, const SparkMatrix4* a, const SparkMatrix4* b) {
    if (out == nullptr || a == nullptr || b == nullptr) {
        return;
    }
    *out = FromMatrix4(ToMatrix4(*a) * ToMatrix4(*b));
}

int spark_mat4_try_invert(SparkMatrix4* out, const SparkMatrix4* m) {
    if (out == nullptr || m == nullptr) {
        return 0;
    }
    Spark::Matrix4 inverted{};
    if (!ToMatrix4(*m).TryInvert(inverted)) {
        return 0;
    }
    *out = FromMatrix4(inverted);
    return 1;
}

void spark_mat4_camera2d_view_projection(
        SparkMatrix4* out,
        const float orthoHalfHeight,
        const float aspect,
        const SparkVector3* cameraPositionWorld) {
    if (out == nullptr) {
        return;
    }
    Spark::Camera2D camera{};
    camera.halfExtentY = orthoHalfHeight;
    if (cameraPositionWorld != nullptr) {
        camera.position = ToVector3(*cameraPositionWorld);
    }
    const float framebufferHeight = orthoHalfHeight * 2.0F;
    const float framebufferWidth = framebufferHeight * aspect;
    *out = FromMatrix4(camera.ViewProjection(framebufferWidth, framebufferHeight));
}

void spark_camera2d_view_projection(
        const SparkCamera2D* camera,
        const float framebufferWidth,
        const float framebufferHeight,
        SparkMatrix4* out) {
    if (camera == nullptr || out == nullptr) {
        return;
    }
    Spark::Camera2D cpp{};
    cpp.position = ToVector3(camera->position);
    cpp.rotationRad = camera->rotationRad;
    cpp.halfExtentY = camera->halfExtentY;
    cpp.clipNearZ = camera->clipNearZ;
    cpp.clipFarZ = camera->clipFarZ;
    *out = FromMatrix4(cpp.ViewProjection(framebufferWidth, framebufferHeight));
}

void spark_camera2d_billboard_basis(
        const SparkCamera2D* camera,
        SparkVector3* outRight,
        SparkVector3* outUp) {
    if (camera == nullptr || outRight == nullptr || outUp == nullptr) {
        return;
    }
    Spark::Camera2D cpp{};
    cpp.position = ToVector3(camera->position);
    cpp.rotationRad = camera->rotationRad;
    cpp.halfExtentY = camera->halfExtentY;
    Spark::Vector3 right{};
    Spark::Vector3 up{};
    cpp.BillboardBasisWorld(right, up);
    *outRight = FromVector3(right);
    *outUp = FromVector3(up);
}

}  // extern "C"
