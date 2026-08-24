#include "spark/scene/assets/gltf/GltfNodeTransforms.hpp"

#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"

#include "cgltf.h"

#include <cmath>

namespace Spark {

namespace {

Matrix4 MatrixFromCgltfColumnMajor(const cgltf_float* values) noexcept {
    Matrix4 m{};
    for (int i = 0; i < 16; ++i) {
        m.m[i] = static_cast<float>(values[i]);
    }
    return m;
}

}  // namespace

Transform Transform::FromAffineMatrix(const Matrix4& matrix) noexcept {
    Transform transform = Transform::Identity;
    transform.translation = {matrix.m[12], matrix.m[13], matrix.m[14]};

    Vector3 basisX{matrix.m[0], matrix.m[1], matrix.m[2]};
    Vector3 basisY{matrix.m[4], matrix.m[5], matrix.m[6]};
    Vector3 basisZ{matrix.m[8], matrix.m[9], matrix.m[10]};

    transform.scale = {basisX.Length(), basisY.Length(), basisZ.Length()};
    if (transform.scale.x > Epsilon) {
        basisX /= transform.scale.x;
    }
    if (transform.scale.y > Epsilon) {
        basisY /= transform.scale.y;
    }
    if (transform.scale.z > Epsilon) {
        basisZ /= transform.scale.z;
    }

    Matrix4 rotationMatrix = Matrix4::IdentityMatrix();
    rotationMatrix.m[0] = basisX.x;
    rotationMatrix.m[1] = basisX.y;
    rotationMatrix.m[2] = basisX.z;
    rotationMatrix.m[4] = basisY.x;
    rotationMatrix.m[5] = basisY.y;
    rotationMatrix.m[6] = basisY.z;
    rotationMatrix.m[8] = basisZ.x;
    rotationMatrix.m[9] = basisZ.y;
    rotationMatrix.m[10] = basisZ.z;

    const float trace = rotationMatrix.m[0] + rotationMatrix.m[5] + rotationMatrix.m[10];
    if (trace > 0.0F) {
        const float s = std::sqrt(trace + 1.0F) * 2.0F;
        transform.rotation.w = 0.25F * s;
        transform.rotation.x = (rotationMatrix.m[6] - rotationMatrix.m[9]) / s;
        transform.rotation.y = (rotationMatrix.m[8] - rotationMatrix.m[2]) / s;
        transform.rotation.z = (rotationMatrix.m[1] - rotationMatrix.m[4]) / s;
    } else if (rotationMatrix.m[0] > rotationMatrix.m[5] && rotationMatrix.m[0] > rotationMatrix.m[10]) {
        const float s = std::sqrt(1.0F + rotationMatrix.m[0] - rotationMatrix.m[5] - rotationMatrix.m[10]) * 2.0F;
        transform.rotation.w = (rotationMatrix.m[6] - rotationMatrix.m[9]) / s;
        transform.rotation.x = 0.25F * s;
        transform.rotation.y = (rotationMatrix.m[4] + rotationMatrix.m[1]) / s;
        transform.rotation.z = (rotationMatrix.m[8] + rotationMatrix.m[2]) / s;
    } else if (rotationMatrix.m[5] > rotationMatrix.m[10]) {
        const float s = std::sqrt(1.0F + rotationMatrix.m[5] - rotationMatrix.m[0] - rotationMatrix.m[10]) * 2.0F;
        transform.rotation.w = (rotationMatrix.m[8] - rotationMatrix.m[2]) / s;
        transform.rotation.x = (rotationMatrix.m[4] + rotationMatrix.m[1]) / s;
        transform.rotation.y = 0.25F * s;
        transform.rotation.z = (rotationMatrix.m[9] + rotationMatrix.m[6]) / s;
    } else {
        const float s = std::sqrt(1.0F + rotationMatrix.m[10] - rotationMatrix.m[0] - rotationMatrix.m[5]) * 2.0F;
        transform.rotation.w = (rotationMatrix.m[1] - rotationMatrix.m[4]) / s;
        transform.rotation.x = (rotationMatrix.m[8] + rotationMatrix.m[2]) / s;
        transform.rotation.y = (rotationMatrix.m[9] + rotationMatrix.m[6]) / s;
        transform.rotation.z = 0.25F * s;
    }
    transform.rotation = transform.rotation.Normalized();
    return transform;
}

Transform GltfNodeTransforms::LocalTransform(const cgltf_node* node) noexcept {
    if (node == nullptr) {
        return Transform::Identity;
    }
    if (node->has_matrix) {
        return Transform::FromAffineMatrix(MatrixFromCgltfColumnMajor(node->matrix));
    }
    Transform transform = Transform::Identity;
    if (node->has_translation) {
        transform.translation = {
                static_cast<float>(node->translation[0]),
                static_cast<float>(node->translation[1]),
                static_cast<float>(node->translation[2])};
    }
    if (node->has_rotation) {
        transform.rotation = Quaternion{
                static_cast<float>(node->rotation[0]),
                static_cast<float>(node->rotation[1]),
                static_cast<float>(node->rotation[2]),
                static_cast<float>(node->rotation[3])};
    }
    if (node->has_scale) {
        transform.scale = {
                static_cast<float>(node->scale[0]),
                static_cast<float>(node->scale[1]),
                static_cast<float>(node->scale[2])};
    }
    return transform;
}

}  // namespace Spark
