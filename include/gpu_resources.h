#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "fdr_parser.h"
#include "geometry.h"

// ── Texture ───────────────────────────────────────────────────
GLuint make_3d_texture(int res);

// ── SSBO for per-vertex lighting ──────────────────────────────
GLuint make_light_ssbo(int n_tets);

// ── Uniform upload helpers ────────────────────────────────────

// Upload uniforms for the lighting compute shader
void upload_lighting_uniforms(
    GLuint program,
    const std::vector<glm::vec4>& vertices,
    const std::vector<Tet>& tetrahedra,
    const glm::vec4& light_pos,
    const glm::vec3& light_color,
    float light_power,
    const glm::vec3& obj_color
);

// Upload uniforms for the ray cast compute shader
void upload_raycast_uniforms(
    GLuint program,
    const std::vector<glm::vec4>& vertices,
    const std::vector<Tet>& tetrahedra,
    const Eye4D& eye,
    int res
);

// Upload uniforms for the render (fragment) shader
void upload_render_uniforms(
    GLuint program,
    const Camera3D& cam,
    float aspect,
    float fov_scale,
    float extinction,
    float step_size
);

// ── Dispatch helpers ──────────────────────────────────────────

void dispatch_lighting(
    GLuint program,
    GLuint ssbo,
    const std::vector<glm::vec4>& vertices,
    const std::vector<Tet>& tetrahedra,
    const glm::vec4& light_pos,
    const glm::vec3& light_color,
    float light_power,
    const glm::vec3& obj_color
);

void dispatch_raycast(
    GLuint program,
    GLuint texture,
    GLuint ssbo,
    const std::vector<glm::vec4>& vertices,
    const std::vector<Tet>& tetrahedra,
    const Eye4D& eye,
    int res
);
