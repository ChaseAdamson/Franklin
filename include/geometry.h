#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "fdr_parser.h"

// ── 4D vector and matrix types ────────────────────────────────
// GLM only goes to 4D natively so we use glm::vec4 and glm::mat4
// for 4D vectors and rotation matrices.

// ── Rotation matrices in 4D planes ───────────────────────────
glm::mat4 rot_xz(float angle);
glm::mat4 rot_xw(float angle);
glm::mat4 rot_zw(float angle);
glm::mat4 rot_xw_legacy(float angle);  // original tesseract rotation

// ── Apply a 4D rotation matrix to a list of vertices ─────────
std::vector<glm::vec4> apply_transform(
    const std::vector<Vec4>& verts,
    const glm::mat4& R,
    const glm::vec4& offset
);

// ── Eye / camera state ────────────────────────────────────────
struct Eye4D {
    glm::vec4 position;
    glm::vec4 b1;       // retinal volume x axis
    glm::vec4 b2;       // retinal volume y axis
    glm::vec4 b3;       // retinal volume z axis
    glm::vec4 focal;    // focal point in 4D space
};

void translate_eye(Eye4D& eye, const glm::vec4& delta);
void apply_rotation(Eye4D& eye, const glm::mat4& R);

// ── 3D orbital camera ─────────────────────────────────────────
struct Camera3D {
    float yaw   =  30.0f;
    float pitch =  20.0f;
    float dist  =   4.0f;

    glm::vec3 position() const;
    glm::vec3 forward()  const;
    glm::vec3 right()    const;
    glm::vec3 up()       const;
};
