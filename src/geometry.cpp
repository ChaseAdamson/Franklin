#include "geometry.h"
#include <cmath>

// ── 4D rotation matrices ──────────────────────────────────────

glm::mat4 rot_xz(float a) {
    float c = cosf(a), s = sinf(a);
    glm::mat4 R(1.0f);
    R[0][0] =  c;  R[2][0] = s;
    R[0][2] = -s;  R[2][2] = c;
    return R;
}

glm::mat4 rot_xw(float a) {
    float c = cosf(a), s = sinf(a);
    glm::mat4 R(1.0f);
    R[0][0] =  c;  R[3][0] = s;
    R[0][3] = -s;  R[3][3] = c;
    return R;
}

glm::mat4 rot_zw(float a) {
    float c = cosf(a), s = sinf(a);
    glm::mat4 R(1.0f);
    R[2][2] =  c;  R[3][2] = s;
    R[2][3] = -s;  R[3][3] = c;
    return R;
}

glm::mat4 rot_xw_legacy(float a) {
    // matches the original Python rotation_matrix_xw
    float c = cosf(a), s = sinf(a);
    glm::mat4 R(1.0f);
    R[0][0] =  c;  R[3][0] = -s;
    R[0][3] =  s;  R[3][3] =  c;
    return R;
}

// ── Apply transform to vertices ───────────────────────────────

std::vector<glm::vec4> apply_transform(
    const std::vector<Vec4>& verts,
    const glm::mat4& R,
    const glm::vec4& offset)
{
    std::vector<glm::vec4> out;
    out.reserve(verts.size());
    for (const auto& v : verts) {
        glm::vec4 gv(v.x, v.y, v.z, v.w);
        out.push_back(R * gv + offset);
    }
    return out;
}

// ── Eye4D ─────────────────────────────────────────────────────

void translate_eye(Eye4D& eye, const glm::vec4& delta) {
    eye.position += delta;
    eye.focal    += delta;
}

void apply_rotation(Eye4D& eye, const glm::mat4& R) {
    eye.b1 = R * eye.b1;
    eye.b2 = R * eye.b2;
    eye.b3 = R * eye.b3;
    glm::vec4 rel = eye.focal - eye.position;
    eye.focal = eye.position + R * rel;
}

// ── Camera3D ──────────────────────────────────────────────────

glm::vec3 Camera3D::position() const {
    float yr = glm::radians(yaw);
    float pr = glm::radians(pitch);
    return glm::vec3(
        dist * cosf(pr) * sinf(yr),
        dist * sinf(pr),
        dist * cosf(pr) * cosf(yr)
    );
}

glm::vec3 Camera3D::forward() const {
    return glm::normalize(-position());
}

glm::vec3 Camera3D::right() const {
    glm::vec3 world_up(0.0f, 1.0f, 0.0f);
    return glm::normalize(glm::cross(forward(), world_up));
}

glm::vec3 Camera3D::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}
